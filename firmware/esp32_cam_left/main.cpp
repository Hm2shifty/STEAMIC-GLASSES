#pragma once
 
bool initCamera();
bool connectToHost();
bool sendFrame();
void cameraLoop();

#pragma once
#include "shared/packet_format.h"

void     initBLE();
void     sendHapticCommand(Priority priority, Direction dir, uint8_t intensity);
void     sendDetectionEvent(DetectionPacket* pkt);
bool     isPhoneConnected();
uint16_t getLatestBPM();

// ─────────────────────────────────────────
//  EchoNav — esp32_cam_left/main.cpp
//  Entry point for the LEFT camera board.
//
//  Boot sequence:
//    1. Serial → WiFi → Camera → BLE → Sensors
//    2. Loop: capture frame → stream to host
//             read IR sensor → trigger haptics
//             receive BLE commands from phone
//
//  The heavy ML work (YOLOv8 + depth) happens
//  on the Python host, NOT on the ESP32.
//  The ESP32 only streams frames and reacts to
//  commands sent back over BLE/WiFi.
// ─────────────────────────────────────────

#include "config.h"
#include "camera.h"
#include "bluetooth.h"
#include "../audio_haptics/bone_audio.h"
#include "../audio_haptics/vibration.h"
#include "../sensors/tof.h"
#include "shared/constants.h"
#include "shared/packet_format.h"

#include <Arduino.h>
#include <WiFi.h>

// ── Timing ───────────────────────────────
static unsigned long lastLoopMs    = 0;
static unsigned long lastSensorMs  = 0;


// ── WiFi setup ───────────────────────────
void connectWifi() {
    Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT_MS) {
            Serial.println("\n[WiFi] Timeout — rebooting");
            ESP.restart();
        }
        delay(250);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] Connected. IP: %s\n",
                  WiFi.localIP().toString().c_str());
}


// ── setup() ──────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== EchoNav LEFT camera board booting ===");

    // 1. Connect to WiFi first (needed for camera stream)
    connectWifi();

    // 2. Init OV2640 camera
    if (!initCamera()) {
        Serial.println("[FATAL] Camera init failed — halting");
        while (true) { delay(1000); }
    }

    // 3. Connect TCP stream to Python host
    if (!connectToHost()) {
        Serial.println("[WARN] Host not reachable — will retry in loop");
    }

    // 4. Init BLE (only left board advertises to phone)
    initBLE();

    // 5. Init haptics and sensors
    initVibration();
    initBoneAudio();
    initToF();

    Serial.println("[BOOT] All systems ready. Starting main loop.");
}


// ── loop() ───────────────────────────────
void loop() {
    unsigned long now = millis();

    // ── Camera frame stream (target 10 fps) ──
    if (now - lastLoopMs >= LOOP_INTERVAL_MS) {
        lastLoopMs = now;
        cameraLoop();   // grab frame and send to host
    }

    // ── Sensor poll (every 50 ms) ─────────
    if (now - lastSensorMs >= SENSOR_POLL_MS) {
        lastSensorMs = now;

        float distM = readToFDistance();   // metres from ToF sensor

        // If something is VERY close, trigger HIGH alert
        // immediately without waiting for the ML host response.
        // This is the "safety override" path described in §3.2 step 5.
        if (distM > 0.0f && distM < DIST_HIGH_M) {
            triggerVibration(PRIORITY_HIGH, DIR_CENTER, VIB_HIGH);
            playBeep(PRIORITY_HIGH);

#if DEBUG_SERIAL
            Serial.printf("[SENSOR] Obstacle at %.2f m — HIGH alert\n", distM);
#endif
        }
    }

    // ── Receive detection commands from Python host ──
    // The Python host sends back a DetectionPacket over
    // a separate TCP command socket (port 8083).
    // For simplicity in this prototype, the left board
    // polls a small HTTP endpoint on the host every loop.
    // (A proper implementation would use a persistent socket.)
    //
    // TODO: replace poll with persistent command socket
    // for lower latency in v2.
}