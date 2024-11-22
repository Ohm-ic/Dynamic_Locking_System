// #include <Base32-Decode.h>
// #include <Wire.h>  
// #include "SH1106Wire.h"
// #include <qrcodeoled.h>
// #include <TOTP.h>
// #include <WiFi.h>
// #include <NetworkClient.h>
// #include <WebServer.h>
// #include <time.h>


// // Wi-Fi Config
// const char* ssid = "ESPP";
// const char* password = "Nasa@2023";

// WebServer server(80);

// // Secret Key
// String secretKey = "JBSWY3DPEHPK3PXP";  // Base32-encoded secret key
// String decodedKey;
// uint8_t hmacKey[20];
// TOTP totp(hmacKey, sizeof(hmacKey), 30); // TOTP with time interval of 30 seconds

// // OLED Display
// SH1106Wire display(0x3c, SDA, SCL);  // OLED display
// QRcodeOled qrcode(&display);  // QR code for OLED

// void setup() {
//   Wire.begin();
//   Serial.begin(115200);

//   // Connect to Wi-Fi
//   WiFi.begin(ssid, password);
//   Serial.print("Connecting to WiFi");
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(1000);
//     Serial.print(".");
//   }
//   Serial.println("\nConnected to Wi-Fi");
//   Serial.println(WiFi.localIP());

//   // Configure SNTP
//   configTime(0, 0, "pool.ntp.org", "time.nist.gov");

//   // Wait for time to sync
//   Serial.println("Waiting for time to sync...");
//   while (time(nullptr) < 1000000000) {  // Wait until epoch time is valid (after 1 Jan 1970)
//     delay(100);
//   }
//   Serial.println("Time synced");

//   // Decode the secret key
//   int result = base32decodeToString(secretKey, decodedKey);
//   if (result < 0) {
//     Serial.println("Failed to Decode the Key");
//     return;
//   }

//   // Convert the decoded key string to a byte array
//   for (size_t i = 0; i < decodedKey.length() && i < sizeof(hmacKey); i++) {
//     hmacKey[i] = static_cast<uint8_t>(decodedKey[i]);
//   }

//   totp = TOTP(hmacKey, sizeof(hmacKey), 30); // Initialize TOTP with the decoded key
//   qrcode.init();  // Initialize QR code display

//    // Define a simple route to respond to HTTP requests
//     server.on("/", HTTP_GET, [](){
//         server.send(200, "text/html", "<h1>Hello ESP 32....</h1>");
//     });

//     // Start the server
//     server.begin();
// }

// void loop() {
//   server.handleClient();
//    CodeGen();
// }

// void CodeGen()
// {
//     // Get the current epoch time
//   time_t now = time(nullptr);
//   int currentSecond = now % 60;  // Extract current second (mod 60 for seconds of the minute)

//   // Wait until the seconds are either 0 or 30 (time intervals)
//   while (currentSecond != 0 && currentSecond != 30) {
//     //delay(100);  // Add a small delay to avoid excessive CPU usage
//     now = time(nullptr);  // Update the current time
//     currentSecond = now % 60;  // Extract new second
//   }

//   // Generate TOTP (Time-based One-Time Password)
//   String generatedTOTP = totp.getCode(now);  // Pass the epoch time directly
//   Serial.print("Generated Timestamp (Epoch): ");
//   Serial.println(now);
//   Serial.println("Generated TOTP: " + generatedTOTP);

//   // Generate QR code and update OLED screen
//   qrcode.create(generatedTOTP.c_str());
//   qrcode.screenupdate();
//   delay(30000);// Adding delay of 30 Sec 
// }





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

// Task handles
TaskHandle_t totpTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;

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
        // Get current TOTP period
        uint32_t currentPeriod = getCurrentTOTPPeriod();
        
        // Check if we've entered a new 30-second period
        if (currentPeriod != lastTOTPPeriod) {
            // Generate new TOTP
            time_t now = time(nullptr);
            String generatedTOTP = totp.getCode(now);
            
            Serial.print("Generated Timestamp (Epoch): ");
            Serial.println(now);
            Serial.println("Generated TOTP: " + generatedTOTP);

            // Take mutex before accessing display
            if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                // Update QR code on display
                qrcode.create(generatedTOTP.c_str());
                qrcode.screenupdate();
                xSemaphoreGive(displayMutex);
            }
            
            // Update last TOTP period
            lastTOTPPeriod = currentPeriod;
        }
        
        // Calculate time until next period
        time_t now = time(nullptr);
        int secondsUntilNext = 30 - (now % 30);
        
        // If we're very close to the next period, use shorter delay
        if (secondsUntilNext <= 1) {
            vTaskDelay(pdMS_TO_TICKS(100)); // Check more frequently near period boundary
        } else {
            vTaskDelay(pdMS_TO_TICKS(500)); // Regular check interval
        }
    }
}

// Web server task
void webServerTask(void *parameter) {
    while (1) {
        server.handleClient();
        // Small delay to prevent watchdog timeout
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    Wire.begin();
    Serial.begin(115200);

    // Create mutex
    displayMutex = xSemaphoreCreateMutex();
    if (displayMutex == NULL) {
        Serial.println("Error creating mutex");
        return;
    }

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi");
    Serial.println(WiFi.localIP());

    // Configure SNTP
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    // Wait for time sync
    Serial.println("Waiting for time to sync...");
    while (time(nullptr) < 1000000000) {
        delay(100);
    }
    Serial.println("Time synced");

    // Decode secret key
    int result = base32decodeToString(secretKey, decodedKey);
    if (result < 0) {
        Serial.println("Failed to Decode the Key");
        return;
    }

    // Convert decoded key to byte array
    for (size_t i = 0; i < decodedKey.length() && i < sizeof(hmacKey); i++) {
        hmacKey[i] = static_cast<uint8_t>(decodedKey[i]);
    }

    // Initialize TOTP and QR code
    totp = TOTP(hmacKey, sizeof(hmacKey), 30);
    qrcode.init();

    // Setup web server routes
    server.on("/", HTTP_GET, [](){
        server.send(200, "text/html", "<h1>Hello ESP 32....</h1>");
    });
    
    // Start web server
    server.begin();

    // Create tasks
    xTaskCreatePinnedToCore(
        totpTask,          // Task function
        "TOTPTask",        // Task name
        8192,             // Stack size (bytes)
        NULL,             // Task parameters
        2,                // Higher priority for TOTP task
        &totpTaskHandle,  // Task handle
        1                 // Core ID (1: Arduino core)
    );

    xTaskCreatePinnedToCore(
        webServerTask,           // Task function
        "WebServerTask",         // Task name 
        8192,                   // Stack size (bytes)
        NULL,                   // Task parameters
        1,                      // Task priority
        &webServerTaskHandle,   // Task handle
        0                       // Core ID (0: Protocol core)
    );
}

void loop() {
    // Empty loop as tasks handle all functionality
    vTaskDelete(NULL);  // Delete setup and loop task
}