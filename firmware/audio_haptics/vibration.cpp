// ─────────────────────────────────────────
//  EchoNav — audio_haptics/vibration.cpp
//  Controls the two coin vibration motors
//  (one per temple) to tell the user which
//  side an object is on.
//
//  OPPOSITE SIDE RULE (paper §3.2 step 7):
//  Object on the RIGHT → LEFT motor vibrates
//  Object on the LEFT  → RIGHT motor vibrates
//  Object in CENTRE    → BOTH motors vibrate
//
//  This feels like something pushing you
//  away from the obstacle, which users
//  find intuitive very quickly.
//
//  Intensity varies with priority:
//  HIGH = strong, MEDIUM = medium, LOW = soft
// ─────────────────────────────────────────

#include "vibration.h"
#include "../shared/constants.h"
#include "../shared/packet_format.h"
#include <Arduino.h>


// ── initVibration() ───────────────────────
// Call once in setup()
void initVibration() {
    // Set up PWM for left motor
    ledcSetup(VIB_LEFT_CH, VIB_FREQ_HZ, VIB_RESOLUTION);
    ledcAttachPin(VIB_LEFT_PIN, VIB_LEFT_CH);
    ledcWrite(VIB_LEFT_CH, 0);   // start off

    // Set up PWM for right motor
    ledcSetup(VIB_RIGHT_CH, VIB_FREQ_HZ, VIB_RESOLUTION);
    ledcAttachPin(VIB_RIGHT_PIN, VIB_RIGHT_CH);
    ledcWrite(VIB_RIGHT_CH, 0);  // start off

    Serial.println("[VIB] Vibration motors ready");
}


// ── stopVibration() ───────────────────────
// Silences both motors immediately
void stopVibration() {
    ledcWrite(VIB_LEFT_CH,  0);
    ledcWrite(VIB_RIGHT_CH, 0);
}


// ── triggerVibration() ────────────────────
// Main function called by the priority engine
// every time a detection event fires.
//
// priority → determines intensity (VIB_HIGH / MEDIUM / LOW)
// dir      → determines which motor fires (opposite side rule)
// intensity → PWM duty 0-255, usually pass VIB_HIGH/MEDIUM/LOW
//
// Motor runs for a short pulse then stops automatically.
void triggerVibration(Priority priority, Direction dir, uint8_t intensity) {

    // Pick duration based on priority
    // HIGH = long buzz, MEDIUM = medium, LOW = short tap
    uint32_t durationMs;
    switch (priority) {
        case PRIORITY_HIGH:   durationMs = 400; break;
        case PRIORITY_MEDIUM: durationMs = 200; break;
        case PRIORITY_LOW:    durationMs = 80;  break;
        default:              durationMs = 100; break;
    }

    // Apply OPPOSITE SIDE RULE
    // Object right → left motor fires (push user left = away from it)
    // Object left  → right motor fires
    // Object centre → both fire equally
    switch (dir) {
        case DIR_RIGHT:
            ledcWrite(VIB_LEFT_CH,  intensity);
            ledcWrite(VIB_RIGHT_CH, 0);
            break;

        case DIR_LEFT:
            ledcWrite(VIB_LEFT_CH,  0);
            ledcWrite(VIB_RIGHT_CH, intensity);
            break;

        case DIR_CENTER:
            ledcWrite(VIB_LEFT_CH,  intensity);
            ledcWrite(VIB_RIGHT_CH, intensity);
            break;
    }

    // Let it run for the duration then stop
    delay(durationMs);
    stopVibration();

#if DEBUG_SERIAL
    const char* dirLabels[]  = { "LEFT", "CENTER", "RIGHT" };
    const char* priLabels[]  = { "LOW", "MEDIUM", "HIGH" };
    Serial.printf("[VIB] Fired — dir: %s | priority: %s | intensity: %d\n",
                  dirLabels[(int)dir], priLabels[(int)priority], intensity);
#endif
}


// ── testMotors() ─────────────────────────
// Runs at boot so the user can feel that
// both motors are working. Fires left then
// right then both for 200ms each.
void testMotors() {
    Serial.println("[VIB] Motor self-test...");

    // Left
    ledcWrite(VIB_LEFT_CH, VIB_MEDIUM);
    delay(200);
    stopVibration();
    delay(100);

    // Right
    ledcWrite(VIB_RIGHT_CH, VIB_MEDIUM);
    delay(200);
    stopVibration();
    delay(100);

    // Both
    ledcWrite(VIB_LEFT_CH,  VIB_MEDIUM);
    ledcWrite(VIB_RIGHT_CH, VIB_MEDIUM);
    delay(200);
    stopVibration();

    Serial.println("[VIB] Self-test done");
}