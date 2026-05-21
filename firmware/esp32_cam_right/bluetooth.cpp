// ─────────────────────────────────────────
//  EchoNav — esp32_cam_right/bluetooth.cpp
//  The right board does NOT advertise BLE
//  to the phone — that's the left board's job.
//
//  This file handles one thing only:
//  Receiving sync commands from the LEFT
//  board so both boards stay coordinated
//  (e.g. reboot together, change frame rate).
//
//  Communication is ESP32 → ESP32 over
//  WiFi UDP — not phone BLE.
//  This file is included to match the
//  project structure but is intentionally
//  minimal.
// ─────────────────────────────────────────

#include "bluetooth.h"
#include "../shared/constants.h"
#include <Arduino.h>
#include <WiFiUdp.h>

// UDP port this board listens on for
// commands from the left board
#define CMD_UDP_PORT 8084

static WiFiUdp udp;
static bool    udpReady = false;

// Last command received
static uint8_t lastCommand = 0;

// ── Command codes ─────────────────────────
#define CMD_REBOOT        0x01
#define CMD_PAUSE_STREAM  0x02
#define CMD_RESUME_STREAM 0x03


// ── initRightBoardComms() ─────────────────
// Call once in setup() after WiFi connects.
// Starts listening for UDP commands from
// the left board.
void initRightBoardComms() {
    udp.begin(CMD_UDP_PORT);
    udpReady = true;
    Serial.printf("[RIGHT-BT] Listening for commands on UDP port %d\n",
                  CMD_UDP_PORT);
}


// ── checkForCommands() ────────────────────
// Call from main loop. Reads any pending
// UDP command from the left board and acts
// on it immediately.
void checkForCommands() {
    if (!udpReady) return;

    int packetSize = udp.parsePacket();
    if (packetSize == 0) return;

    uint8_t cmd = 0;
    udp.read(&cmd, 1);
    lastCommand = cmd;

    switch (cmd) {
        case CMD_REBOOT:
            Serial.println("[RIGHT-BT] Reboot command received");
            delay(100);
            ESP.restart();
            break;

        case CMD_PAUSE_STREAM:
            Serial.println("[RIGHT-BT] Pause stream command received");
            // main loop checks isPaused() before sending frames
            break;

        case CMD_RESUME_STREAM:
            Serial.println("[RIGHT-BT] Resume stream command received");
            break;

        default:
            Serial.printf("[RIGHT-BT] Unknown command: 0x%02X\n", cmd);
            break;
    }
}


// ── isPaused() ────────────────────────────
// Returns true if the left board told this
// board to pause its camera stream.
bool isPaused() {
    return lastCommand == CMD_PAUSE_STREAM;
}