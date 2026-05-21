// ─────────────────────────────────────────
//  EchoNav — esp32_cam_left/camera.cpp
//  Initialises the OV2640 camera and
//  streams MJPEG frames over WiFi to the
//  Python ML host for stereo processing.
// ─────────────────────────────────────────

#include "camera.h"
#include "config.h"
#include "esp_camera.h"
#include "esp_log.h"
#include <WiFi.h>
#include <WiFiClient.h>

static const char* TAG = "CAM_L";

// TCP client — stays open for the whole session
static WiFiClient streamClient;


// ── initCamera() ─────────────────────────
// Call once in setup(). Configures the
// OV2640 and returns true on success.
bool initCamera() {
    camera_config_t config;

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;

    // Pin assignments from constants.h
    config.pin_d0    = CAM_PIN_D0;
    config.pin_d1    = CAM_PIN_D1;
    config.pin_d2    = CAM_PIN_D2;
    config.pin_d3    = CAM_PIN_D3;
    config.pin_d4    = CAM_PIN_D4;
    config.pin_d5    = CAM_PIN_D5;
    config.pin_d6    = CAM_PIN_D6;
    config.pin_d7    = CAM_PIN_D7;
    config.pin_xclk  = CAM_PIN_XCLK;
    config.pin_pclk  = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;
    config.pin_href  = CAM_PIN_HREF;
    config.pin_sccb_sda = CAM_PIN_SIOD;
    config.pin_sccb_scl = CAM_PIN_SIOC;
    config.pin_pwdn  = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;

    config.xclk_freq_hz = 20000000;   // 20 MHz XCLK
    config.pixel_format = PIXFORMAT_JPEG;

    // QVGA 320x240 — low enough to hit 10fps on ESP32
    config.frame_size   = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;   // 0=best, 63=worst — 12 is a good balance
    config.fb_count     = 2;    // double buffer reduces frame drops

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed: 0x%x", err);
        return false;
    }

    // Apply mirror/flip settings from config.h
    sensor_t* sensor = esp_camera_sensor_get();
    sensor->set_hmirror(sensor, CAM_HMIRROR ? 1 : 0);
    sensor->set_vflip(sensor,   CAM_VFLIP   ? 1 : 0);

    ESP_LOGI(TAG, "Camera (%s) ready", CAM_LABEL);
    return true;
}


// ── connectToHost() ───────────────────────
// Opens a TCP connection to the Python host.
// Call after WiFi is connected.
bool connectToHost() {
    if (streamClient.connect(HOST_IP, STREAM_PORT)) {
        ESP_LOGI(TAG, "Connected to host %s:%d", HOST_IP, STREAM_PORT);
        return true;
    }
    ESP_LOGE(TAG, "Cannot reach host");
    return false;
}


// ── sendFrame() ──────────────────────────
// Grab one JPEG frame and send it over TCP.
// The Python host reads frames in a loop.
//
// Frame format sent:
//   [4 bytes: frame length as uint32_t big-endian]
//   [N bytes: JPEG data]
//
// Returns false if the connection dropped.
bool sendFrame() {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGW(TAG, "Frame capture failed");
        return true;   // not a connection error, keep going
    }

    if (!streamClient.connected()) {
        esp_camera_fb_return(fb);
        return false;   // signal caller to reconnect
    }

    // Send 4-byte length header so host knows how many bytes to read
    uint32_t len = fb->len;
    uint8_t header[4] = {
        (uint8_t)(len >> 24),
        (uint8_t)(len >> 16),
        (uint8_t)(len >>  8),
        (uint8_t)(len      )
    };
    streamClient.write(header, 4);

    // Send JPEG payload
    streamClient.write(fb->buf, fb->len);

    esp_camera_fb_return(fb);

#if DEBUG_SERIAL
    Serial.printf("[CAM %s] sent %u bytes\n", CAM_LABEL, len);
#endif

    return true;
}


// ── cameraLoop() ─────────────────────────
// Call from main loop every LOOP_INTERVAL_MS.
// Handles reconnection automatically.
void cameraLoop() {
    if (!streamClient.connected()) {
        ESP_LOGW(TAG, "Reconnecting to host...");
        streamClient.stop();
        delay(500);
        connectToHost();
        return;
    }
    sendFrame();
}