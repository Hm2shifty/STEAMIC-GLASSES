// ─────────────────────────────────────────
//  EchoNav — esp32_cam_left/bluetooth.cpp
//  Sets up a BLE server so the Android
//  app can receive detection alerts and
//  send back heart rate data.
//
//  Uses the NimBLE-Arduino library (lighter
//  than the default BLE stack on ESP32).
//  Install: Arduino Library Manager → NimBLE-Arduino
// ─────────────────────────────────────────

#include "bluetooth.h"
#include "config.h"
#include "shared/constants.h"
#include "shared/packet_format.h"
#include <NimBLEDevice.h>

static const char* TAG = "BLE_L";

// BLE objects
static NimBLEServer*         bleServer    = nullptr;
static NimBLECharacteristic* hapticChar   = nullptr;
static NimBLECharacteristic* priorityChar = nullptr;
static NimBLECharacteristic* hrChar       = nullptr;

// Tracks whether the phone is connected
static bool phoneConnected = false;

// Latest heart rate received from phone (bpm)
static uint16_t latestBPM = 0;


// ── Connection callbacks ──────────────────
// Called by BLE stack when phone connects/disconnects
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* server) override {
        phoneConnected = true;
        Serial.println("[BLE] Phone connected");
    }
    void onDisconnect(NimBLEServer* server) override {
        phoneConnected = false;
        Serial.println("[BLE] Phone disconnected — restarting advertising");
        NimBLEDevice::startAdvertising();
    }
};


// ── Heart rate write callback ─────────────
// Called when the phone writes a new BPM value
class HRCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* chr) override {
        std::string val = chr->getValue();
        if (val.size() >= sizeof(HeartRatePacket)) {
            HeartRatePacket pkt;
            memcpy(&pkt, val.data(), sizeof(HeartRatePacket));
            latestBPM = pkt.bpm;
#if DEBUG_SERIAL
            Serial.printf("[BLE] Heart rate: %d bpm (session %d)\n",
                          pkt.bpm, pkt.session_id);
#endif
        }
    }
};


// ── initBLE() ────────────────────────────
// Call once in setup() after serial is ready.
void initBLE() {
    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);   // max TX power

    bleServer = NimBLEDevice::createServer();
    bleServer->setCallbacks(new ServerCallbacks());

    // Create the main service
    NimBLEService* service = bleServer->createService(BLE_SERVICE_UUID);

    // Characteristic 1: Haptic commands (ESP32 → phone, notify)
    hapticChar = service->createCharacteristic(
        BLE_CHAR_HAPTIC_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    // Characteristic 2: Detection events (ESP32 → phone, notify)
    priorityChar = service->createCharacteristic(
        BLE_CHAR_PRIORITY_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    // Characteristic 3: Heart rate (phone → ESP32, write)
    hrChar = service->createCharacteristic(
        BLE_CHAR_HEARTRATE_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    hrChar->setCallbacks(new HRCallbacks());

    service->start();

    // Advertise so the phone can find "EchoNav"
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
    adv->addServiceUUID(BLE_SERVICE_UUID);
    adv->setScanResponse(true);
    NimBLEDevice::startAdvertising();

    Serial.println("[BLE] Advertising as EchoNav");
}


// ── sendHapticCommand() ───────────────────
// Fired every time the priority engine decides
// the user needs a haptic alert.
// Also triggers the local motors directly via
// audio_haptics code — this just notifies the phone.
void sendHapticCommand(Priority priority, Direction dir, uint8_t intensity) {
    if (!phoneConnected) return;

    HapticPacket pkt;
    pkt.priority  = (uint8_t)priority;
    pkt.direction = (uint8_t)dir;
    pkt.intensity = intensity;
    pkt.reserved  = 0;

    hapticChar->setValue((uint8_t*)&pkt, sizeof(HapticPacket));
    hapticChar->notify();
}


// ── sendDetectionEvent() ─────────────────
// Sends a full detection packet to the phone
// so the app can display it in the live feed.
void sendDetectionEvent(DetectionPacket* pkt) {
    if (!phoneConnected) return;

    priorityChar->setValue((uint8_t*)pkt, sizeof(DetectionPacket));
    priorityChar->notify();
}


// ── isPhoneConnected() ───────────────────
bool isPhoneConnected() {
    return phoneConnected;
}


// ── getLatestBPM() ───────────────────────
uint16_t getLatestBPM() {
    return latestBPM;
}