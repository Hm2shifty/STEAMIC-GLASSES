#pragma once
#include "shared/constants.h"

// ─────────────────────────────────────────
//  EchoNav — esp32_cam_left/config.h
//  Board-specific settings for the LEFT
//  camera ESP32. The right camera board
//  has its own config.h with different
//  port and camera ID values.
// ─────────────────────────────────────────


// ── Which camera this board is ───────────
#define CAM_ID          0          // 0 = left, 1 = right
#define CAM_LABEL       "LEFT"

// This board streams on the LEFT port
#define STREAM_PORT     STREAM_PORT_L   // 8081 from constants.h


// ── Camera orientation ───────────────────
// The left OV2640 is mounted mirrored
// so flip it horizontally to get a
// natural left-eye perspective.
#define CAM_HMIRROR     true
#define CAM_VFLIP       false


// ── BLE role ─────────────────────────────
// Only the LEFT board runs BLE and talks
// to the phone. The right board sends its
// frames to the left board via WiFi only.
#define IS_BLE_MASTER   true


// ── Haptic motors on this board ──────────
// Left board controls BOTH motors because
// BLE commands come in here first, then
// the left board fires the correct motor.
#define HAS_HAPTICS     true


// ── IR sensor on this board ──────────────
// IR proximity sensor is mounted on the
// left side of the frame (paper §3.1)
#define HAS_IR_SENSOR   true


// ── Bone conduction on this board ────────
// Bone conduction audio driven from left board
#define HAS_BONE_AUDIO  true


// ── Debug serial output ──────────────────
// Set to true during development to print
// detection events to Serial Monitor (115200 baud)
#define DEBUG_SERIAL    true