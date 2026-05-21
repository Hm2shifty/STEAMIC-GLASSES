// ─────────────────────────────────────────
//  EchoNav — esp32_cam_right/main.cpp
//  Entry point for the RIGHT camera board.
//
//  This board does ONE thing:
//    Connect to WiFi → stream frames to
//    the Python host on port 8082.
//
//  All ML, BLE, haptics, and sensors are
//  handled by the LEFT board. This board
//  just provides the right-eye image for
//  the stereo depth pipeline.
// ─────────────────────────────────────────

#include "config.h"
#include "camera.h"
#include "bluetooth.h"
#include "../shared/constants.h"

#include <Arduino.h>
#include <WiFi.h>

static unsigned long lastLoopMs = 0;


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
    Serial.println("\n=== EchoNav RIGHT camera board booting ===");

    // 1. WiFi
    connectWifi();

    // 2. Camera
    if (!initCamera()) {
        Serial.println("[FATAL] Camera init failed — halting");
        while (true) { delay(1000); }
    }

    // 3. Connect stream to Python host
    if (!connectToHost()) {
        Serial.println("[WARN] Host not reachable — will retry in loop");
    }

    // 4. Init command listener from left board
    initRightBoardComms();

    Serial.println("[BOOT] Right camera ready. Streaming...");
}


// ── loop() ───────────────────────────────
// Simple — just stream frames at 10 fps.
// No sensors, no BLE, no haptics.
void loop() {
    unsigned long now = millis();

    if (now - lastLoopMs >= LOOP_INTERVAL_MS) {
        lastLoopMs = now;
        checkForCommands();          // check for commands from left board
        if (!isPaused()) {
            cameraLoop();            // only stream if not paused
        }
    }
}