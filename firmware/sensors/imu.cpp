// ─────────────────────────────────────────
//  EchoNav — sensors/imu.cpp
//  Reads motion data from the IMU sensor
//  mounted in the glasses frame.
//
//  Used for two things:
//  1. Detect if user is turning their head
//     so the priority engine knows which
//     direction they are facing
//  2. Detect if the glasses have been taken
//     off (no movement for a long time =
//     pause alerts so they don't annoy anyone
//     nearby)
//
//  Uses the MPU6050 (accelerometer + gyro)
//  over I2C. Install library:
//  Arduino Library Manager → MPU6050 by ElectronicCats
// ─────────────────────────────────────────

#include "imu.h"
#include "../shared/constants.h"
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>

static MPU6050 mpu;

// Stores the last known heading so we can
// detect how much the head has turned
static float lastYaw = 0.0f;

// If no significant movement for this long
// the glasses are assumed to be off (ms)
#define STILL_TIMEOUT_MS   5000
#define MOVEMENT_THRESHOLD 0.5f   // degrees per second

static unsigned long lastMovementMs = 0;


// ── initIMU() ────────────────────────────
// Call once in setup()
bool initIMU() {
    Wire.begin();
    mpu.initialize();

    if (!mpu.testConnection()) {
        Serial.println("[IMU] MPU6050 not found — check wiring");
        return false;
    }

    // Set gyro range to ±250°/s (most sensitive)
    mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
    // Set accel range to ±2g
    mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);

    lastMovementMs = millis();
    Serial.println("[IMU] MPU6050 ready");
    return true;
}


// ── readIMU() ────────────────────────────
// Returns the current gyroscope Z-axis
// reading in degrees per second.
// Positive = turning right, negative = left.
float readIMU() {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Convert raw gyro Z to degrees per second
    // MPU6050 at ±250°/s range: sensitivity = 131 LSB/°/s
    float gyroZ = gz / 131.0f;

    // Track last time we saw movement
    if (abs(gyroZ) > MOVEMENT_THRESHOLD) {
        lastMovementMs = millis();
    }

#if DEBUG_SERIAL
    Serial.printf("[IMU] GyroZ: %.2f deg/s\n", gyroZ);
#endif

    return gyroZ;
}


// ── isGlassesOn() ────────────────────────
// Returns false if no movement detected
// for STILL_TIMEOUT_MS — glasses are
// probably sitting on a table.
bool isGlassesOn() {
    return (millis() - lastMovementMs < STILL_TIMEOUT_MS);
}


// ── getHeadTurnDirection() ───────────────
// Returns which way the user is turning
// their head based on gyro Z reading.
// Used by the priority engine to adjust
// which side to alert on.
Direction getHeadTurnDirection() {
    float gyroZ = readIMU();

    if (gyroZ > 15.0f)  return DIR_RIGHT;
    if (gyroZ < -15.0f) return DIR_LEFT;
    return DIR_CENTER;
}