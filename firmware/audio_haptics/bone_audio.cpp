// ─────────────────────────────────────────
//  EchoNav — audio_haptics/bone_audio.cpp
//  Drives the bone conduction transducer
//  with different beep tones depending on
//  the priority level of a detected object.
//
//  RED    (HIGH)   → fast high-pitched beep
//  YELLOW (MEDIUM) → medium beep
//  GREEN  (LOW)    → slow low-pitched beep
//
//  Uses ESP32 LEDC (PWM) to generate tones.
//  No external audio library needed.
// ─────────────────────────────────────────

#include "bone_audio.h"
#include "../shared/constants.h"
#include "../shared/packet_format.h"
#include <Arduino.h>

// Tracks when the last beep fired so we
// don't overlap beeps (cooldown period)
static unsigned long lastBeepMs = 0;


// ── initBoneAudio() ───────────────────────
// Call once in setup()
void initBoneAudio() {
    // Set up LEDC PWM channel for bone conduction pin
    ledcSetup(BONE_CH, TONE_LOW_HZ, BONE_RESOLUTION);
    ledcAttachPin(BONE_PIN, BONE_CH);

    // Start silent
    ledcWrite(BONE_CH, 0);

    Serial.println("[AUDIO] Bone conduction ready");
}


// ── playTone() ────────────────────────────
// Internal helper — plays a tone at a given
// frequency for a given duration then stops.
static void playTone(uint32_t freqHz, uint32_t durationMs) {
    ledcSetup(BONE_CH, freqHz, BONE_RESOLUTION);
    ledcWrite(BONE_CH, 128);   // 50% duty = loudest square wave
    delay(durationMs);
    ledcWrite(BONE_CH, 0);     // silence
}


// ── playBeep() ────────────────────────────
// Call this whenever a detection event fires.
// Picks the right frequency + duration based
// on priority level (paper §3.2 step 6).
//
// Respects BEEP_COOLDOWN_MS so beeps don't
// stack on top of each other.
void playBeep(Priority priority) {
    unsigned long now = millis();

    // Don't beep if we just beeped
    if (now - lastBeepMs < BEEP_COOLDOWN_MS) return;
    lastBeepMs = now;

    switch (priority) {
        case PRIORITY_HIGH:
            // RED — urgent, high pitch, short sharp beep
            // Play it twice so user definitely notices
            playTone(TONE_HIGH_HZ, BEEP_HIGH_MS);
            delay(80);
            playTone(TONE_HIGH_HZ, BEEP_HIGH_MS);
            break;

        case PRIORITY_MEDIUM:
            // YELLOW — one medium beep
            playTone(TONE_MED_HZ, BEEP_MED_MS);
            break;

        case PRIORITY_LOW:
            // GREEN — one soft low beep
            playTone(TONE_LOW_HZ, BEEP_LOW_MS);
            break;

        default:
            break;
    }

#if DEBUG_SERIAL
    const char* labels[] = { "LOW", "MEDIUM", "HIGH" };
    Serial.printf("[AUDIO] Beep — priority: %s\n", labels[(int)priority]);
#endif
}


// ── playStartupChime() ────────────────────
// Plays a short ascending tone when the
// glasses boot successfully so the user
// knows the device is ready without looking
// at a screen.
void playStartupChime() {
    playTone(TONE_LOW_HZ,  100);
    delay(60);
    playTone(TONE_MED_HZ,  100);
    delay(60);
    playTone(TONE_HIGH_HZ, 150);
    Serial.println("[AUDIO] Startup chime played");
}