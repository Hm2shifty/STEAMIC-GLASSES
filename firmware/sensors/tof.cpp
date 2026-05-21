// ─────────────────────────────────────────
//  EchoNav — sensors/tof.cpp
//  Reads distance from the HC-SR04
//  ultrasonic sensor mounted on the
//  left side of the glasses frame.
//
//  This is the "safety override" sensor —
//  if something is within 1 metre it fires
//  a HIGH alert immediately without waiting
//  for the ML pipeline to respond.
//
//  How HC-SR04 works:
//  1. Send a 10µs pulse on TRIG pin
//  2. Measure how long ECHO pin stays HIGH
//  3. Distance = (echo time × speed of sound) / 2
// ─────────────────────────────────────────

#include "tof.h"
#include "../shared/constants.h"
#include <Arduino.h>

// Speed of sound in cm per microsecond
#define SOUND_SPEED_CM_US  0.0343f

// Max readable distance for HC-SR04 (cm)
#define MAX_DISTANCE_CM    400.0f

// If echo takes longer than this, no object detected (µs)
#define TIMEOUT_US         25000


// ── initToF() ────────────────────────────
// Call once in setup()
void initToF() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    pinMode(IR_PIN,   INPUT);

    // Make sure trig starts LOW
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    Serial.println("[TOF] Ultrasonic sensor ready");
}


// ── readToFDistance() ────────────────────
// Fires the HC-SR04 and returns distance
// in METRES. Returns -1.0 if no object
// detected within range.
float readToFDistance() {
    // 1. Send 10µs trigger pulse
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // 2. Measure echo pulse duration
    unsigned long duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);

    // 3. Timeout means nothing in range
    if (duration == 0) return -1.0f;

    // 4. Convert to centimetres
    float distanceCm = (duration * SOUND_SPEED_CM_US) / 2.0f;

    // 5. Clamp to valid range
    if (distanceCm > MAX_DISTANCE_CM) return -1.0f;

    // 6. Convert to metres and return
    float distanceM = distanceCm / 100.0f;

#if DEBUG_SERIAL
    Serial.printf("[TOF] Distance: %.2f m\n", distanceM);
#endif

    return distanceM;
}


// ── readIR() ─────────────────────────────
// Reads the IR proximity sensor.
// Returns true if something is very close
// (within IR_CLOSE_CM from constants.h).
// Used as a backup for low-contrast surfaces
// like glass that the ultrasonic can miss.
bool readIR() {
    // IR sensor outputs LOW when object detected
    bool detected = (digitalRead(IR_PIN) == LOW);

#if DEBUG_SERIAL
    if (detected) Serial.println("[IR] Close obstacle detected");
#endif

    return detected;
}