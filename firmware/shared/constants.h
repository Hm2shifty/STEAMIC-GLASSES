#pragma once

// ─────────────────────────────────────────
//  EchoNav — shared/constants.h
//  All hardware pin numbers, thresholds,
//  timing values, and BLE UUIDs used
//  across both ESP32 boards live here.
//  Change a value here → it updates everywhere.
// ─────────────────────────────────────────


// ── Camera (OV2640) ──────────────────────
// These pins follow the standard AI-Thinker
// ESP32-CAM board layout.
#define CAM_PIN_PWDN    32
#define CAM_PIN_RESET   -1   // tied to EN on AI-Thinker board
#define CAM_PIN_XCLK     0
#define CAM_PIN_SIOD    26   // SDA
#define CAM_PIN_SIOC    27   // SCL
#define CAM_PIN_D7      35
#define CAM_PIN_D6      34
#define CAM_PIN_D5      39
#define CAM_PIN_D4      36
#define CAM_PIN_D3      21
#define CAM_PIN_D2      19
#define CAM_PIN_D1      18
#define CAM_PIN_D0       5
#define CAM_PIN_VSYNC   25
#define CAM_PIN_HREF    23
#define CAM_PIN_PCLK    22

// Frame size sent over WiFi for stereo pipeline.
// QVGA (320x240) keeps latency low on ESP32.
#define CAM_FRAME_WIDTH  320
#define CAM_FRAME_HEIGHT 240


// ── IR / Ultrasonic sensor ───────────────
// HC-SR04 ultrasonic — left-side mount
#define TRIG_PIN        12
#define ECHO_PIN        13
// IR proximity — close obstacle flag
#define IR_PIN          14

// Distance below which IR overrides camera (cm)
#define IR_CLOSE_CM     40


// ── Haptic motors ────────────────────────
// Left temple vibration motor (PWM channel 0)
#define VIB_LEFT_PIN    15
#define VIB_LEFT_CH      0
// Right temple vibration motor (PWM channel 1)
#define VIB_RIGHT_PIN    2
#define VIB_RIGHT_CH     1

// PWM settings for vibration motors
#define VIB_FREQ_HZ    5000
#define VIB_RESOLUTION    8   // 8-bit → 0–255 duty

// Vibration intensity per priority level (0–255)
#define VIB_HIGH   230
#define VIB_MEDIUM 140
#define VIB_LOW     60


// ── Bone conduction audio ────────────────
// Bone conduction transducer driven via PWM tone
#define BONE_PIN         4
#define BONE_CH          2
#define BONE_RESOLUTION  8

// Beep tone frequencies (Hz) per priority
#define TONE_HIGH_HZ   1200
#define TONE_MED_HZ     800
#define TONE_LOW_HZ     400

// Beep duration (ms) per priority
#define BEEP_HIGH_MS    300
#define BEEP_MED_MS     200
#define BEEP_LOW_MS     100

// Minimum gap between beeps so they don't overlap (ms)
#define BEEP_COOLDOWN_MS 500


// ── Priority thresholds ──────────────────
// Distances in metres — match the paper spec:
//   RED    (HIGH)   < 1.0 m
//   YELLOW (MEDIUM) 1.0 – 3.0 m
//   GREEN  (LOW)    > 3.0 m
#define DIST_HIGH_M   1.0f
#define DIST_MEDIUM_M 3.0f

// If object is moving faster than this toward user
// it gets bumped up one priority level (paper §4.1)
#define APPROACH_SPEED_MPS 1.0f


// ── Stereo depth formula constants ───────
// Z = (FOCAL_PX * BASELINE_M) / disparity_px
// Baseline = distance between the two camera lenses
// (~65 mm to match human inter-ocular distance)
#define BASELINE_M    0.065f
// Focal length in pixels for OV2640 at QVGA
// (approximate — calibrate for your exact lens)
#define FOCAL_PX     160.0f


// ── WiFi (frame streaming) ───────────────
// Left camera streams to the Python ML host.
// Right camera streams to the same host on a
// different port so they can be grabbed together.
#define WIFI_SSID     "EchoNav_Net"
#define WIFI_PASS     "echonav2026"

#define HOST_IP       "192.168.4.1"   // Python host IP
#define STREAM_PORT_L  8081           // left  cam port
#define STREAM_PORT_R  8082           // right cam port

// How long to wait for WiFi before rebooting (ms)
#define WIFI_TIMEOUT_MS 10000


// ── BLE (phone app communication) ────────
// UUIDs generated once and shared by both boards
// and the Android app.
#define BLE_SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define BLE_CHAR_HAPTIC_UUID    "12345678-1234-1234-1234-123456789ab1"
#define BLE_CHAR_PRIORITY_UUID  "12345678-1234-1234-1234-123456789ab2"
#define BLE_CHAR_HEARTRATE_UUID "12345678-1234-1234-1234-123456789ab3"
#define BLE_DEVICE_NAME         "EchoNav"


// ── Timing ───────────────────────────────
// Detection loop target: 5–10 fps (paper §4.2)
// 100 ms per loop = 10 fps
#define LOOP_INTERVAL_MS  100

// How often to poll the ultrasonic sensor (ms)
#define SENSOR_POLL_MS     50