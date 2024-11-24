/**
*@author  Hariom Agrahari (hariomagrahari06@gmail.com)
*@brief   Arduino Sketch with MQTT Support
*@version 1.2.0
*@date 2024-11-24
*
*@copyright CopyRight (c) 2024
*
*/
#include <Base32-Decode.h>
#include <Wire.h>
#include "SH1106Wire.h"
#include <qrcodeoled.h>
#include <TOTP.h>
#include <WiFi.h>
#include <NetworkClient.h>
#include <time.h>
#include <PubSubClient.h>  // Add MQTT library

// Wi-Fi Config
const char* ssid = "ESPP";
const char* password = "Nasa@2023";

// MQTT Configuration
const char* mqtt_server = "mqtt.eclipseprojects.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/subscribe";

// MQTT Client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Secret Key
String secretKey = "JBSWY3DPEHPK3PXP";  // Base32-encoded secret key
String decodedKey;
uint8_t hmacKey[20];
TOTP totp(hmacKey, sizeof(hmacKey), 30);  // TOTP with time interval of 30 seconds

// OLED Display
SH1106Wire display(0x3c, SDA, SCL);  // OLED display
QRcodeOled qrcode(&display);         // QR code for OLED

// Relay and Push Button Pins
#define RELAY_PIN 5
#define BUTTON_PIN 17

// Task handles
TaskHandle_t totpTaskHandle = NULL;
TaskHandle_t buttonRelayTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;

// Mutex for protecting shared resources
SemaphoreHandle_t displayMutex;
SemaphoreHandle_t relayMutex;

// Global Flag to Track the Relay task
bool relayActive = false;

// Keep track of last TOTP period
uint32_t lastTOTPPeriod = 0;

// Function to get current TOTP period (time / 30)
uint32_t getCurrentTOTPPeriod() {
  return time(nullptr) / 30;
}

// Function to activate relay
void activateRelay() {
  relayActive = true;  // Setting up of flag
  if (xSemaphoreTake(relayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
      display.displayOff();  // Turn off the display
      xSemaphoreGive(displayMutex);
    }

    digitalWrite(RELAY_PIN, LOW);      // Turn on the relay
    vTaskDelay(pdMS_TO_TICKS(10000));  // Wait for 10 seconds
    digitalWrite(RELAY_PIN, HIGH);     // Turn off the relay

    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
      display.displayOn();  // Turn on the display
      xSemaphoreGive(displayMutex);
    }

    xSemaphoreGive(relayMutex);
  }
  relayActive = false;  // Flag Reset
}

// MQTT callback function
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.println("Message received: " + message);

  if (message == "ON") {
    // display.displayOff();
    Serial.println("MQTT command received! Activating relay.");
    activateRelay();
    // display.displayOn();
  }
}

// MQTT connection function
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("Attempting MQTT connection...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT Broker");
      mqttClient.subscribe(mqtt_topic);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retrying in 5 seconds");
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}

// MQTT task
void mqttTask(void* parameter) {
  while (1) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// TOTP generation task
void totpTask(void* parameter) {
    while (1) {
        if (!relayActive) {  // Check relay status before generating TOTP
            uint32_t currentPeriod = getCurrentTOTPPeriod();
            if (currentPeriod != lastTOTPPeriod) {
                time_t now = time(nullptr);
                String generatedTOTP = totp.getCode(now);

                if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    qrcode.create(generatedTOTP.c_str());
                    qrcode.screenupdate();
                    xSemaphoreGive(displayMutex);
                }
                lastTOTPPeriod = currentPeriod;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));  // Regular delay
    }
}

// Button and relay control task
void buttonRelayTask(void* parameter) {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  while (1) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button Pressed! Activating relay.");
      activateRelay();
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  Wire.begin();
  Serial.begin(115200);

  displayMutex = xSemaphoreCreateMutex();
  relayMutex = xSemaphoreCreateMutex();
  if (displayMutex == NULL || relayMutex == NULL) {
    Serial.println("Error creating mutexes");
    return;
  }

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.println(WiFi.localIP());

  // Setup MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);

  // Setup time
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for time to sync...");
  while (time(nullptr) < 1000000000) {
    delay(100);
  }
  Serial.println("Time synced");

  // Setup TOTP
  int result = base32decodeToString(secretKey, decodedKey);
  if (result < 0) {
    Serial.println("Failed to Decode the Key");
    return;
  }

  for (size_t i = 0; i < decodedKey.length() && i < sizeof(hmacKey); i++) {
    hmacKey[i] = static_cast<uint8_t>(decodedKey[i]);
  }

  totp = TOTP(hmacKey, sizeof(hmacKey), 30);
  qrcode.init();

  // Create tasks on different cores
  xTaskCreatePinnedToCore(
    totpTask,
    "TOTPTask",
    8192,
    NULL,
    2,
    &totpTaskHandle,
    1);

  xTaskCreatePinnedToCore(
    buttonRelayTask,
    "ButtonRelayTask",
    4096,
    NULL,
    1,
    &buttonRelayTaskHandle,
    0);

  xTaskCreatePinnedToCore(
    mqttTask,
    "MQTTTask",
    4096,
    NULL,
    1,
    &mqttTaskHandle,
    0);
}

void loop() {
  vTaskDelete(NULL);
}