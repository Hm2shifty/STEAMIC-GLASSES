// ─────────────────────────────────────────
//  EchoNav — android/bluetooth/BLEManager.js
//  Handles all Bluetooth Low Energy comms
//  between the phone and the ESP32.
//
//  What it does:
//  - Scans for "EchoNav" device
//  - Connects to it
//  - Subscribes to detection notifications
//  - Subscribes to haptic command notifications
//  - Sends heart rate data to the ESP32
//
//  Uses react-native-ble-plx library.
//  Install: npm install react-native-ble-plx
// ─────────────────────────────────────────

import { BleManager } from 'react-native-ble-plx';
import { Buffer } from 'buffer';

// ── BLE UUIDs (must match constants.h) ───
const SERVICE_UUID        = '12345678-1234-1234-1234-123456789abc';
const CHAR_HAPTIC_UUID    = '12345678-1234-1234-1234-123456789ab1';
const CHAR_PRIORITY_UUID  = '12345678-1234-1234-1234-123456789ab2';
const CHAR_HEARTRATE_UUID = '12345678-1234-1234-1234-123456789ab3';

// ── Priority labels ───────────────────────
export const PRIORITY_LABELS = {
    0: 'LOW',
    1: 'MEDIUM',
    2: 'HIGH'
};

export const PRIORITY_COLORS = {
    0: '#2ecc71',   // GREEN
    1: '#f39c12',   // YELLOW
    2: '#e74c3c'    // RED
};

export const DIR_LABELS = {
    0: 'LEFT',
    1: 'CENTER',
    2: 'RIGHT'
};

// ── Object class labels ───────────────────
const OBJECT_LABELS = {
    0: 'person',   1: 'bicycle', 2: 'car',
    3: 'motorcycle', 4: 'bus',   5: 'truck',
    6: 'door',     7: 'stairs',  8: 'curb',
    9: 'obstacle'
};


// ─────────────────────────────────────────
//  BLEManager class
//  One instance shared across the whole app
// ─────────────────────────────────────────
class BLEManager {

    constructor() {
        this.manager    = new BleManager();
        this.device     = null;
        this.connected  = false;

        // Callbacks set by screens
        this.onDetection    = null;   // called with parsed DetectionPacket
        this.onHaptic       = null;   // called with parsed HapticPacket
        this.onDisconnect   = null;   // called when device disconnects
    }


    // ── scan() ───────────────────────────
    // Scans for the EchoNav ESP32 and
    // connects when found.
    scan(onFound) {
        console.log('[BLE] Scanning for EchoNav...');

        this.manager.startDeviceScan(null, null, (error, device) => {
            if (error) {
                console.error('[BLE] Scan error:', error);
                return;
            }

            if (device && device.name === 'EchoNav') {
                this.manager.stopDeviceScan();
                console.log('[BLE] Found EchoNav — connecting...');
                this.connect(device.id, onFound);
            }
        });
    }


    // ── connect() ────────────────────────
    async connect(deviceId, onConnected) {
        try {
            this.device = await this.manager.connectToDevice(deviceId);
            await this.device.discoverAllServicesAndCharacteristics();

            this.connected = true;
            console.log('[BLE] Connected to EchoNav');

            // Handle disconnection
            this.device.onDisconnected(() => {
                this.connected = false;
                console.log('[BLE] Disconnected');
                if (this.onDisconnect) this.onDisconnect();
            });

            // Subscribe to detection events
            this._subscribeToDetections();
            this._subscribeToHaptics();

            if (onConnected) onConnected();

        } catch (error) {
            console.error('[BLE] Connect error:', error);
        }
    }


    // ── _subscribeToDetections() ─────────
    // Listens for DetectionPacket notifications
    // from the ESP32 (12 bytes)
    _subscribeToDetections() {
        this.device.monitorCharacteristicForService(
            SERVICE_UUID,
            CHAR_PRIORITY_UUID,
            (error, characteristic) => {
                if (error) return;

                const bytes = Buffer.from(
                    characteristic.value, 'base64'
                );

                // Parse DetectionPacket (matches packet_format.h)
                // uint8 priority, uint8 direction, uint16 class,
                // float distance, float speed
                const priority  = bytes.readUInt8(0);
                const direction = bytes.readUInt8(1);
                const classId   = bytes.readUInt16BE(2);
                const distance  = bytes.readFloatBE(4);
                const speed     = bytes.readFloatBE(8);

                const packet = {
                    priority,
                    direction,
                    classId,
                    label:    OBJECT_LABELS[classId] || 'unknown',
                    distance: distance.toFixed(2),
                    speed:    speed.toFixed(2),
                    color:    PRIORITY_COLORS[priority],
                    timestamp: new Date().toLocaleTimeString()
                };

                if (this.onDetection) this.onDetection(packet);
            }
        );
    }


    // ── _subscribeToHaptics() ────────────
    // Listens for HapticPacket notifications
    // (4 bytes) — used to mirror haptic
    // events on screen for accessibility
    _subscribeToHaptics() {
        this.device.monitorCharacteristicForService(
            SERVICE_UUID,
            CHAR_HAPTIC_UUID,
            (error, characteristic) => {
                if (error) return;

                const bytes = Buffer.from(
                    characteristic.value, 'base64'
                );

                const packet = {
                    priority:  bytes.readUInt8(0),
                    direction: bytes.readUInt8(1),
                    intensity: bytes.readUInt8(2)
                };

                if (this.onHaptic) this.onHaptic(packet);
            }
        );
    }


    // ── sendHeartRate() ──────────────────
    // Sends BPM reading from the wrist
    // sensor to the ESP32 for logging.
    // HeartRatePacket: uint16 bpm, uint8 session, uint8 reserved
    async sendHeartRate(bpm, sessionId) {
        if (!this.connected || !this.device) return;

        try {
            const buf = Buffer.alloc(4);
            buf.writeUInt16BE(bpm,       0);
            buf.writeUInt8(sessionId,    2);
            buf.writeUInt8(0,            3);   // reserved

            await this.device.writeCharacteristicWithResponseForService(
                SERVICE_UUID,
                CHAR_HEARTRATE_UUID,
                buf.toString('base64')
            );

            console.log(`[BLE] Sent BPM: ${bpm} (session ${sessionId})`);

        } catch (error) {
            console.error('[BLE] sendHeartRate error:', error);
        }
    }


    // ── disconnect() ─────────────────────
    async disconnect() {
        if (this.device) {
            await this.device.cancelConnection();
            this.connected = false;
            console.log('[BLE] Manually disconnected');
        }
    }

    isConnected() {
        return this.connected;
    }
}

// Export a single shared instance
export default new BLEManager();