#pragma once
#include "../shared/constants.h"

// ─────────────────────────────────────────
//  EchoNav — esp32_cam_right/config.h
//  Board-specific settings for the RIGHT
//  camera ESP32.
//
//  Differences from left board:
//   - Streams on port 8082 (not 8081)
//   - No BLE (left board handles phone comms)
//   - No IR sensor (left side only)
//   - No bone audio (left board only)
//   - No haptic motors (left board only)
//   - Camera is NOT mirrored (right-eye view)
// ─────────────────────────────────────────


// ── Which camera this board is ───────────
#define CAM_ID          1          // 0 = left, 1 = right
#define CAM_LABEL       "RIGHT"

// Right board streams on the RIGHT port
#define STREAM_PORT     STREAM_PORT_R   // 8082 from constants.h


// ── Camera orientation ───────────────────
// Right camera is NOT mirrored — natural
// right-eye perspective
#define CAM_HMIRROR     false
#define CAM_VFLIP       false


// ── This board does NOT handle these ─────
#define IS_BLE_MASTER   false
#define HAS_HAPTICS     false
#define HAS_IR_SENSOR   false
#define HAS_BONE_AUDIO  false


// ── Debug serial output ──────────────────
#define DEBUG_SERIAL    true