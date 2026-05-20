#pragma once
#include <stdint.h>

// ─────────────────────────────────────────
//  EchoNav — shared/packet_format.h
//  Defines the exact byte layout of every
//  BLE message sent between the ESP32 and
//  the Android app.
//
//  Both sides (ESP32 + Android) must use
//  the same struct layout, so keep this
//  file in sync with bluetooth_protocol.md
// ─────────────────────────────────────────


// ── Priority levels ───────────────────────
// Matches the RED / YELLOW / GREEN system
// described in the paper (§3.2 step 6)
typedef enum {
    PRIORITY_LOW    = 0,   // GREEN  — > 3.0 m
    PRIORITY_MEDIUM = 1,   // YELLOW — 1.0–3.0 m
    PRIORITY_HIGH   = 2    // RED    — < 1.0 m
} Priority;


// ── Object direction ─────────────────────
// Which side of the camera frame the object
// is detected in. Used to pick which motor
// vibrates (opposite side rule from paper §3.2)
typedef enum {
    DIR_LEFT   = 0,
    DIR_CENTER = 1,
    DIR_RIGHT  = 2
} Direction;


// ── Haptic command packet ─────────────────
// Sent from ESP32 → phone (BLE_CHAR_HAPTIC_UUID)
// Also used internally to trigger motors directly.
//
// Total size: 4 bytes
#pragma pack(push, 1)
typedef struct {
    uint8_t   priority;    // Priority enum value (0/1/2)
    uint8_t   direction;   // Direction enum value (0/1/2)
    uint8_t   intensity;   // Motor PWM duty 0–255
    uint8_t   reserved;    // Padding — set to 0
} HapticPacket;
#pragma pack(pop)


// ── Detection event packet ────────────────
// Sent from ESP32 → phone (BLE_CHAR_PRIORITY_UUID)
// Phone app displays this in its live feed.
//
// Total size: 12 bytes
#pragma pack(push, 1)
typedef struct {
    uint8_t   priority;        // Priority enum
    uint8_t   direction;       // Direction enum
    uint16_t  object_class;    // YOLO class index (0–79 for COCO)
    float     distance_m;      // Estimated depth in metres
    float     approach_speed;  // Positive = moving toward user (m/s)
} DetectionPacket;
#pragma pack(pop)


// ── Heart rate packet ─────────────────────
// Sent from phone → ESP32 (BLE_CHAR_HEARTRATE_UUID)
// Logged during user testing sessions (paper §3.2 step 8)
//
// Total size: 4 bytes
#pragma pack(push, 1)
typedef struct {
    uint16_t  bpm;         // Heart rate in beats per minute
    uint8_t   session_id;  // Which test session (1-based)
    uint8_t   reserved;    // Padding — set to 0
} HeartRatePacket;
#pragma pack(pop)


// ── YOLO class names (navigation-relevant subset) ──
// Full COCO has 80 classes. We only alert on these.
// Index matches the .tflite model output.
static const char* OBJECT_LABELS[] = {
    "person",       // 0
    "bicycle",      // 1
    "car",          // 2
    "motorcycle",   // 3
    "bus",          // 4
    "truck",        // 5
    "door",         // 6  (custom fine-tuned class)
    "stairs",       // 7  (custom fine-tuned class)
    "curb",         // 8  (custom fine-tuned class)
    "obstacle"      // 9  (custom fine-tuned class)
};
#define NUM_LABELS 10