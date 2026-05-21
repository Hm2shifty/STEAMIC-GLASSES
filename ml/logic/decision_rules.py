# ─────────────────────────────────────────
#  EchoNav — ml/logic/decision_rules.py
#  The priority engine. Takes raw YOLO
#  detections + depth estimates and decides:
#
#  RED    (HIGH)   → object < 1.0 m
#  YELLOW (MEDIUM) → object 1.0 – 3.0 m
#  GREEN  (LOW)    → object > 3.0 m
#
#  Also bumps priority UP one level if the
#  object is approaching faster than 1 m/s
#  (paper §4.1 — time-to-collision logic)
#
#  Output is sent back to the ESP32 over
#  a TCP socket so it can trigger haptics.
# ─────────────────────────────────────────

import time
import socket
import struct

# ── Priority constants ────────────────────
PRIORITY_LOW    = 0   # GREEN
PRIORITY_MEDIUM = 1   # YELLOW
PRIORITY_HIGH   = 2   # RED

# ── Distance thresholds (metres) ─────────
DIST_HIGH_M   = 1.0
DIST_MEDIUM_M = 3.0

# ── Approach speed threshold (m/s) ───────
# If object closes faster than this, bump
# up one priority level (paper §4.1)
APPROACH_SPEED_MPS = 1.0

# ── Direction zones ───────────────────────
# Camera frame is 320px wide (QVGA)
# Left third / middle third / right third
DIR_LEFT   = 0
DIR_CENTER = 1
DIR_RIGHT  = 2

FRAME_WIDTH = 320

# ── Object classes we care about ─────────
# Everything else is ignored to reduce noise
RELEVANT_CLASSES = {
    0:  "person",
    1:  "bicycle",
    2:  "car",
    3:  "motorcycle",
    4:  "bus",
    5:  "truck",
    6:  "door",
    7:  "stairs",
    8:  "curb",
    9:  "obstacle"
}

# ── Command socket back to ESP32 ──────────
ESP32_IP   = "192.168.4.2"   # left ESP32 IP on the network
ESP32_PORT = 8083            # command port (ESP32 listens here)


# ─────────────────────────────────────────
#  Detection class
#  Holds one detected object and all its
#  computed properties
# ─────────────────────────────────────────
class Detection:
    def __init__(self, class_id, bbox, distance_m, timestamp):
        """
        class_id    : YOLO class index (int)
        bbox        : (x1, y1, x2, y2) bounding box in pixels
        distance_m  : depth estimate in metres (float)
        timestamp   : time.time() when this was detected
        """
        self.class_id      = class_id
        self.bbox          = bbox
        self.distance_m    = distance_m
        self.timestamp     = timestamp
        self.approach_speed = 0.0   # filled in by tracker
        self.priority      = PRIORITY_LOW
        self.direction     = DIR_CENTER

    def label(self):
        return RELEVANT_CLASSES.get(self.class_id, "unknown")

    def center_x(self):
        x1, y1, x2, y2 = self.bbox
        return (x1 + x2) / 2


# ─────────────────────────────────────────
#  Priority engine
# ─────────────────────────────────────────
class PriorityEngine:

    def __init__(self):
        # Stores previous detections to compute approach speed
        # key = class_id, value = (distance_m, timestamp)
        self.prev_detections = {}

    def compute_direction(self, center_x):
        """
        Divides the 320px frame into 3 zones:
        LEFT (0-106), CENTER (107-213), RIGHT (214-320)
        """
        third = FRAME_WIDTH / 3
        if center_x < third:
            return DIR_LEFT
        elif center_x < third * 2:
            return DIR_CENTER
        else:
            return DIR_RIGHT

    def compute_approach_speed(self, class_id, current_dist, current_time):
        """
        Estimates how fast the object is closing in m/s.
        Positive = moving toward user.
        """
        if class_id not in self.prev_detections:
            self.prev_detections[class_id] = (current_dist, current_time)
            return 0.0

        prev_dist, prev_time = self.prev_detections[class_id]
        dt = current_time - prev_time

        # Avoid division by zero
        if dt < 0.001:
            return 0.0

        # Positive speed means getting closer
        speed = (prev_dist - current_dist) / dt

        # Update stored value
        self.prev_detections[class_id] = (current_dist, current_time)

        return speed

    def classify(self, detection):
        """
        Takes a Detection object and assigns:
        - detection.priority  (LOW / MEDIUM / HIGH)
        - detection.direction (LEFT / CENTER / RIGHT)
        - detection.approach_speed

        Returns the updated Detection.
        """
        now = time.time()

        # 1. Compute approach speed
        detection.approach_speed = self.compute_approach_speed(
            detection.class_id,
            detection.distance_m,
            now
        )

        # 2. Assign base priority from distance
        if detection.distance_m < DIST_HIGH_M:
            priority = PRIORITY_HIGH
        elif detection.distance_m < DIST_MEDIUM_M:
            priority = PRIORITY_MEDIUM
        else:
            priority = PRIORITY_LOW

        # 3. Bump up one level if approaching fast (paper §4.1)
        if detection.approach_speed > APPROACH_SPEED_MPS:
            priority = min(priority + 1, PRIORITY_HIGH)

        detection.priority  = priority
        detection.direction = self.compute_direction(detection.center_x())

        return detection

    def process_frame(self, detections):
        """
        Takes a list of Detection objects from one frame,
        classifies each one, filters out irrelevant classes,
        and returns only the highest priority detections
        sorted by urgency.

        detections : list of Detection objects
        returns    : sorted list (highest priority first)
        """
        results = []

        for det in detections:
            # Skip objects we don't care about
            if det.class_id not in RELEVANT_CLASSES:
                continue

            # Skip objects with no valid depth reading
            if det.distance_m <= 0:
                continue

            self.classify(det)
            results.append(det)

        # Sort by priority descending (HIGH first)
        results.sort(key=lambda d: d.priority, reverse=True)

        return results


# ─────────────────────────────────────────
#  ESP32 command sender
#  Sends the top detection back to the
#  ESP32 so it can fire haptics + audio
# ─────────────────────────────────────────
def send_to_esp32(detection):
    """
    Packs a DetectionPacket and sends it to
    the ESP32 command socket.

    Packet format (12 bytes, matches packet_format.h):
      uint8  priority
      uint8  direction
      uint16 object_class
      float  distance_m
      float  approach_speed
    """
    try:
        data = struct.pack(
            ">BBHff",
            detection.priority,
            detection.direction,
            detection.class_id,
            detection.distance_m,
            detection.approach_speed
        )
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(0.1)   # don't block the ML loop
        sock.connect((ESP32_IP, ESP32_PORT))
        sock.sendall(data)
        sock.close()

    except Exception as e:
        # Don't crash the ML loop if ESP32 is unreachable
        print(f"[SEND] Could not reach ESP32: {e}")


# ─────────────────────────────────────────
#  Priority label helpers (for logging)
# ─────────────────────────────────────────
PRIORITY_LABELS = {
    PRIORITY_LOW:    "GREEN  (LOW)",
    PRIORITY_MEDIUM: "YELLOW (MEDIUM)",
    PRIORITY_HIGH:   "RED    (HIGH)"
}

DIR_LABELS = {
    DIR_LEFT:   "LEFT",
    DIR_CENTER: "CENTER",
    DIR_RIGHT:  "RIGHT"
}

def log_detection(detection):
    print(
        f"[DETECT] {detection.label():<10} | "
        f"{PRIORITY_LABELS[detection.priority]} | "
        f"dist: {detection.distance_m:.2f}m | "
        f"speed: {detection.approach_speed:.2f}m/s | "
        f"dir: {DIR_LABELS[detection.direction]}"
    )