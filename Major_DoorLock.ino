
// Includes for libraries
#include <Base32-Decode.h>
#include <Wire.h>  
#include "SH1106Wire.h"
#include <qrcodeoled.h>
#include <TOTP.h>
#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <time.h>

// Wi-Fi Config
const char* ssid = "ESPP";
const char* password = "Nasa@2023";

WebServer server(80);

// Secret Key
String secretKey = "JBSWY3DPEHPK3PXP";  // Base32-encoded secret key
String decodedKey;
uint8_t hmacKey[20];
TOTP totp(hmacKey, sizeof(hmacKey), 30); // TOTP with time interval of 30 seconds

// OLED Display
SH1106Wire display(0x3c, SDA, SCL);  // OLED display
QRcodeOled qrcode(&display);  // QR code for OLED

// Relay and Push Button Pins
#define  RELAY_PIN  5
#define BUTTON_PIN 17



// Task handles
TaskHandle_t totpTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;
TaskHandle_t buttonRelayTaskHandle = NULL;

// Mutex for protecting shared resources
SemaphoreHandle_t displayMutex;

// Keep track of last TOTP period
uint32_t lastTOTPPeriod = 0;

// Function to get current TOTP period (time / 30)
uint32_t getCurrentTOTPPeriod() {
    return time(nullptr) / 30;
}

// TOTP generation task
void totpTask(void *parameter) {
    while (1) {
        uint32_t currentPeriod = getCurrentTOTPPeriod();
        if (currentPeriod != lastTOTPPeriod) {
            time_t now = time(nullptr);
            String generatedTOTP = totp.getCode(now);

            Serial.print("Generated Timestamp (Epoch): ");
            Serial.println(now);
            Serial.println("Generated TOTP: " + generatedTOTP);

            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
              display.setBrightness(50);
                qrcode.create(generatedTOTP.c_str());
                qrcode.screenupdate();
                xSemaphoreGive(displayMutex);
            }
            lastTOTPPeriod = currentPeriod;
        }

        time_t now = time(nullptr);
        int secondsUntilNext = 30 - (now % 30);

        if (secondsUntilNext <= 1) {
            vTaskDelay(pdMS_TO_TICKS(100));
        } else {
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

// Web server task
void webServerTask(void *parameter) {
    while (1) {
        server.handleClient();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// Button and relay control task
void buttonRelayTask(void *parameter) {
    pinMode(BUTTON_PIN, INPUT_PULLUP);  // Configure button pin as input with pull-up
    pinMode(RELAY_PIN, OUTPUT);        // Configure relay pin as output
    digitalWrite(RELAY_PIN, HIGH);      // Ensure relay is off initially
   
    while (1) {
        if (digitalRead(BUTTON_PIN) == LOW) {  // Check if button is pressed
            Serial.println("Button Pressed! Activating relay and turning off display.");

            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                display.displayOff();  // Turn off the display
                xSemaphoreGive(displayMutex);
            }

            digitalWrite(RELAY_PIN, LOW);  // Turn on the relay
            vTaskDelay(pdMS_TO_TICKS(10000));  // Wait for 10 seconds

            digitalWrite(RELAY_PIN, HIGH);  // Turn off the relay

            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                display.displayOn();  // Turn on the display
                xSemaphoreGive(displayMutex);
            }

            Serial.println("Relay deactivated, display turned back on.");
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Small delay for debouncing
    }

  
}

void setup() {
    Wire.begin();
    Serial.begin(115200);

    displayMutex = xSemaphoreCreateMutex();
    if (displayMutex == NULL) {
        Serial.println("Error creating mutex");
        return;
    }

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi");
    Serial.println(WiFi.localIP());

    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("Waiting for time to sync...");
    while (time(nullptr) < 1000000000) {
        delay(100);
    }
    Serial.println("Time synced");

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

    server.on("/", HTTP_GET, [](){
        server.send(200, "text/html", "<h1>Hello ESP 32....</h1>");
    });

    server.begin();

    xTaskCreatePinnedToCore(
        totpTask,
        "TOTPTask",
        8192,
        NULL,
        2,
        &totpTaskHandle,
        1
    );

    xTaskCreatePinnedToCore(
        webServerTask,
        "WebServerTask",
        8192,
        NULL,
        1,
        &webServerTaskHandle,
        0
    );

    xTaskCreatePinnedToCore(
        buttonRelayTask,
        "ButtonRelayTask",
        4096,
        NULL,
        1,
        &buttonRelayTaskHandle,
        0
    );
}

void loop() {
    vTaskDelete(NULL);
}
