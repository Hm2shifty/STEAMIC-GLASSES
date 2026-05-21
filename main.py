# ─────────────────────────────────────────
#  EchoNav — main.py
#  The main ML pipeline. Runs on the host
#  computer (or Raspberry Pi / laptop) and:
#
#  1. Receives MJPEG frames from both ESP32s
#  2. Runs YOLOv8 object detection
#  3. Computes stereo depth (distance)
#  4. Runs priority engine (RED/YELLOW/GREEN)
#  5. Sends alerts back to ESP32 over TCP
#
#  Start this BEFORE powering the glasses.
#  Run: python main.py
# ─────────────────────────────────────────

import cv2
import numpy as np
import socket
import struct
import threading
import time
from ultralytics import YOLO

from ml.logic.decision_rules import (
    PriorityEngine,
    Detection,
    send_to_esp32,
    log_detection,
    RELEVANT_CLASSES,
    PRIORITY_HIGH,
    PRIORITY_MEDIUM,
    PRIORITY_LOW,
    DIR_LEFT,
    DIR_CENTER,
    DIR_RIGHT
)

# ── Settings ──────────────────────────────
# IP this machine listens on
HOST_IP        = "0.0.0.0"

# Ports the two ESP32 cameras stream to
PORT_LEFT      = 8081
PORT_RIGHT     = 8082

# YOLOv8 model path
# Use the TFLite version exported for ESP32
# or the full .pt version for host inference
MODEL_PATH     = "ml/models/object_detection.tflite"

# Stereo depth constants (must match constants.h)
BASELINE_M     = 0.065    # 65mm between lenses
FOCAL_PX       = 160.0    # focal length in pixels at QVGA

# Frame size (must match ESP32 camera config)
FRAME_W        = 320
FRAME_H        = 240

# Show live detection window (set False on headless server)
SHOW_DISPLAY   = True

# Colours for bounding boxes
BOX_COLORS = {
    PRIORITY_HIGH:   (0,   0,   255),   # RED
    PRIORITY_MEDIUM: (0,   165, 255),   # ORANGE/YELLOW
    PRIORITY_LOW:    (0,   200, 0  ),   # GREEN
}


# ─────────────────────────────────────────
#  Frame receiver
#  Listens on a TCP port and reads frames
#  sent by one ESP32 camera board.
#  Runs in its own thread.
# ─────────────────────────────────────────
class FrameReceiver:

    def __init__(self, port, label):
        self.port    = port
        self.label   = label
        self.frame   = None      # latest decoded frame (numpy array)
        self.lock    = threading.Lock()
        self.running = False

    def start(self):
        self.running = True
        t = threading.Thread(target=self._listen, daemon=True)
        t.start()
        print(f"[{self.label}] Listening on port {self.port}")

    def _listen(self):
        server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((HOST_IP, self.port))
        server.listen(1)

        while self.running:
            print(f"[{self.label}] Waiting for ESP32 connection...")
            conn, addr = server.accept()
            print(f"[{self.label}] ESP32 connected from {addr}")

            try:
                self._receive_frames(conn)
            except Exception as e:
                print(f"[{self.label}] Connection lost: {e}")
            finally:
                conn.close()

    def _receive_frames(self, conn):
        """
        Reads frames in the format sent by camera.cpp:
        [4 bytes: length][N bytes: JPEG data]
        """
        while self.running:
            # Read 4-byte length header
            header = self._recv_exact(conn, 4)
            if not header:
                break

            length = struct.unpack(">I", header)[0]

            # Read JPEG payload
            data = self._recv_exact(conn, length)
            if not data:
                break

            # Decode JPEG to numpy array
            jpg   = np.frombuffer(data, dtype=np.uint8)
            frame = cv2.imdecode(jpg, cv2.IMREAD_COLOR)

            if frame is not None:
                with self.lock:
                    self.frame = frame

    def _recv_exact(self, conn, n):
        """Read exactly n bytes from socket"""
        buf = b""
        while len(buf) < n:
            chunk = conn.recv(n - len(buf))
            if not chunk:
                return None
            buf += chunk
        return buf

    def get_frame(self):
        with self.lock:
            return self.frame.copy() if self.frame is not None else None


# ─────────────────────────────────────────
#  Stereo depth estimator
#  Computes distance using the disparity
#  between left and right camera frames.
#  Z = (focal_px * baseline_m) / disparity
# ─────────────────────────────────────────
class StereoDepth:

    def __init__(self):
        # StereoBM is fast enough for real-time on a laptop
        self.stereo = cv2.StereoBM_create(numDisparities=64, blockSize=15)

    def compute(self, frame_left, frame_right):
        """
        Returns a disparity map (numpy array).
        Each pixel value = estimated disparity in pixels.
        """
        gray_l = cv2.cvtColor(frame_left,  cv2.COLOR_BGR2GRAY)
        gray_r = cv2.cvtColor(frame_right, cv2.COLOR_BGR2GRAY)

        disparity = self.stereo.compute(gray_l, gray_r).astype(np.float32)
        # StereoBM returns values x16 — divide to get real disparity
        disparity /= 16.0
        return disparity

    def get_distance(self, disparity_map, bbox):
        """
        Gets the median disparity inside a bounding box,
        then converts to distance in metres.
        Z = (focal_px * baseline_m) / disparity
        """
        x1, y1, x2, y2 = [int(v) for v in bbox]

        # Clamp to frame bounds
        x1 = max(0, x1); y1 = max(0, y1)
        x2 = min(FRAME_W - 1, x2); y2 = min(FRAME_H - 1, y2)

        roi = disparity_map[y1:y2, x1:x2]

        # Filter out invalid disparity values
        valid = roi[roi > 0]
        if len(valid) == 0:
            return -1.0

        median_disp = np.median(valid)
        if median_disp <= 0:
            return -1.0

        distance_m = (FOCAL_PX * BASELINE_M) / median_disp
        return float(distance_m)


# ─────────────────────────────────────────
#  Main pipeline
# ─────────────────────────────────────────
def main():
    print("\n=== EchoNav ML Pipeline Starting ===\n")

    # ── Load YOLO model ───────────────────
    print("[1/4] Loading YOLOv8 model...")
    try:
        model = YOLO(MODEL_PATH)
        print(f"      Model loaded: {MODEL_PATH}")
    except Exception as e:
        print(f"[ERROR] Could not load model: {e}")
        print("        Make sure ml/models/object_detection.tflite exists")
        return

    # ── Start frame receivers ─────────────
    print("[2/4] Starting frame receivers...")
    left_rx  = FrameReceiver(PORT_LEFT,  "LEFT")
    right_rx = FrameReceiver(PORT_RIGHT, "RIGHT")
    left_rx.start()
    right_rx.start()

    # ── Init depth and priority engine ────
    print("[3/4] Initialising depth + priority engine...")
    depth_engine    = StereoDepth()
    priority_engine = PriorityEngine()

    print("[4/4] Pipeline ready. Waiting for frames...\n")

    # ── Main loop ─────────────────────────
    frame_count = 0
    while True:
        loop_start = time.time()

        # Get latest frames from both cameras
        frame_l = left_rx.get_frame()
        frame_r = right_rx.get_frame()

        # Wait until both cameras are sending
        if frame_l is None or frame_r is None:
            time.sleep(0.05)
            continue

        frame_count += 1

        # ── Run YOLO on left frame ─────────
        results = model(frame_l, verbose=False)[0]
        detections = []

        # ── Compute disparity map ──────────
        disparity = depth_engine.compute(frame_l, frame_r)

        # ── Build Detection objects ────────
        for box in results.boxes:
            class_id = int(box.cls[0])

            # Skip irrelevant classes
            if class_id not in RELEVANT_CLASSES:
                continue

            bbox       = box.xyxy[0].tolist()   # [x1, y1, x2, y2]
            distance_m = depth_engine.get_distance(disparity, bbox)

            det = Detection(
                class_id   = class_id,
                bbox       = bbox,
                distance_m = distance_m,
                timestamp  = time.time()
            )
            detections.append(det)

        # ── Run priority engine ────────────
        ranked = priority_engine.process_frame(detections)

        # ── Send top detection to ESP32 ────
        if ranked:
            top = ranked[0]
            log_detection(top)
            send_to_esp32(top)

        # ── Draw detections on display ─────
        if SHOW_DISPLAY:
            display = frame_l.copy()
            for det in ranked:
                x1, y1, x2, y2 = [int(v) for v in det.bbox]
                color = BOX_COLORS.get(det.priority, (255, 255, 255))
                label = f"{det.label()} {det.distance_m:.1f}m"

                cv2.rectangle(display, (x1, y1), (x2, y2), color, 2)
                cv2.putText(display, label,
                            (x1, y1 - 8),
                            cv2.FONT_HERSHEY_SIMPLEX,
                            0.5, color, 1)

            # Show FPS
            fps = 1.0 / max(time.time() - loop_start, 0.001)
            cv2.putText(display, f"FPS: {fps:.1f}",
                        (5, 20),
                        cv2.FONT_HERSHEY_SIMPLEX,
                        0.6, (255, 255, 255), 1)

            cv2.imshow("EchoNav — Left Camera", display)

            if cv2.waitKey(1) & 0xFF == ord("q"):
                print("\n[INFO] Quit key pressed — shutting down")
                break

    cv2.destroyAllWindows()
    print("[INFO] Pipeline stopped.")


if __name__ == "__main__":
    main()