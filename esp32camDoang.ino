#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <esp_now.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Replace with your network credentials
const char* ssid = "Jancoeghaseyo 2.0";
const char* password = "Terserahlugakpeduli";

// Firebase Storage Configuration
const char* firebaseHost = "firebasestorage.googleapis.com";
const char* firebaseAuth = "h0biMsta5bJEy4ninf3Q9wYcYxbXbO7yoE9DhvMK";  // Your Firebase authentication token

// Structure to receive data
typedef struct struct_message {
  int sensorValue;
} struct_message;

struct_message incomingData;

String sensorData = "No Data";

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incoming, int len) {
  if (len == sizeof(struct_message)) {
    const struct_message* incomingData = (const struct_message*)incoming;
    sensorData = String(incomingData->sensorValue);
    Serial.printf("Sensor Value Received: %d\n", incomingData->sensorValue);
  }
}


void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  // Configure camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_CIF;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW initialized and callback registered");
}

void loop() {
  delay(60000 - (millis() % 60000));  // Wait for the next minute
  sendVideoFrames();
}

void sendVideoFrames() {
    int frameCount = 0;
    int totalFrames = 30 * 10;  // 30 fps * 10 seconds

    while (frameCount < totalFrames) {
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Camera capture failed");
            continue;
        }

        if (WiFi.status() == WL_CONNECTED) {
            WiFiClientSecure client;
            client.setInsecure();  // Use this with caution in production
            HTTPClient http;

            String path = "/v0/b/tesdssdsiksokpd.appspot.com/o/frames%2Fframe" + String(frameCount) + ".jpg?uploadType=media&name=frames/frame" + String(frameCount) + ".jpg";
            String url = String("https://") + firebaseHost + path;

            http.begin(client, url);
            http.addHeader("Content-Type", "image/jpeg");

            int httpResponseCode = http.POST(fb->buf, fb->len);
            if (httpResponseCode > 0) {
                Serial.printf("Frame %d sent successfully\n", frameCount);
            } else {
                Serial.printf("Error sending frame %d: %s\n", frameCount, http.errorToString(httpResponseCode).c_str());
            }

            http.end();

            //cek frame udh di akhir blom
            if (frameCount == totalFrames - 1) {
                // Send sensor data along with the last frame
                String dataPath = "/v0/b/tesdssdsiksokpd.appspot.com/o/data%2Fframe" + String(frameCount) + ".json?uploadType=media&name=data/frame" + String(frameCount) + ".json";
                String dataUrl = String("https://") + firebaseHost + dataPath;

                http.begin(client, dataUrl);
                http.addHeader("Content-Type", "application/json");

                String jsonData = "{\"sensorValue\": \"" + sensorData + "\"}";
                httpResponseCode = http.POST((uint8_t*)jsonData.c_str(), jsonData.length());
                if (httpResponseCode > 0) {
                    Serial.printf("Data for frame %d sent successfully\n", frameCount);
                } else {
                    Serial.printf("Error sending data for frame %d: %s\n", frameCount, http.errorToString(httpResponseCode).c_str());
                }

                http.end();
            }
        }

        esp_camera_fb_return(fb);
        frameCount++;
        delay(1000 / 30);  // Maintain 30 fps
    }
}
