Here is a detailed README file for your project:

---

# ESP32 TOTP-Based Door Lock System

This project implements a Time-based One-Time Password (TOTP) system on an ESP32 module to control a door lock. The project integrates features like MQTT communication, an SH1106 OLED display, and hardware button inputs. 

## Features
- **Wi-Fi Connectivity:** The ESP32 connects to a Wi-Fi network for NTP-based time synchronization and MQTT communication.
- **TOTP Authentication:** Generates a TOTP every 30 seconds based on a pre-shared secret key.
- **SH1106 OLED Display:** Displays the current TOTP as a QR code.
- **Relay Control:** Activates the relay to unlock the door upon button press or receiving an MQTT command.
- **Multi-Tasking:** Utilizes FreeRTOS tasks for efficient resource utilization.
- **Hardware Mutexes:** Ensures synchronized access to shared resources like the display and relay.

## Hardware Setup
### Components
- **ESP32 Module**
- **SH1106 OLED Display**
  - **SDA:** GPIO 21
  - **SCL:** GPIO 22
- **Relay Module**
  - **Control Pin:** GPIO 5
- **Push Button**
  - **Input Pin:** GPIO 17

### Wiring Diagram
| Component           | ESP32 Pin |
|---------------------|-----------|
| OLED SDA            | GPIO 21   |
| OLED SCL            | GPIO 22   |
| Relay Control Pin   | GPIO 5    |
| Button Input        | GPIO 17   |

## Software Details

### Dependencies
The project uses the following libraries:
- **Wi-Fi:** For network connectivity.
- **MQTT:** For message communication.
- **NTP:** For time synchronization.
- **TOTP:** For generating one-time passwords.
- **SH1106 OLED Display:** For displaying the TOTP and QR codes.
- **Base32 Decoder:** For decoding the TOTP secret key.

### Configuration

#### Wi-Fi Configuration
Update the following variables in the code:
```cpp
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";
```

#### MQTT Configuration
Set up the MQTT broker details:
```cpp
const char* mqtt_server = "mqtt.eclipseprojects.io";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/subscribe";
```

#### TOTP Secret Key
Replace the placeholder secret key with your Base32-encoded secret:
```cpp
String secretKey = "JBSWY3DPEHPK3PXP";
```

### FreeRTOS Tasks
- **`totpTask`**
  - Generates and displays the TOTP QR code on the OLED every 30 seconds.
- **`buttonRelayTask`**
  - Monitors the button and activates the relay for 10 seconds when pressed.
- **`mqttTask`**
  - Handles MQTT communication and relay activation based on received messages.

### Key Functions
1. **TOTP Generation:**
   - Generates a new TOTP every 30 seconds using the NTP-synced time.
2. **MQTT Control:**
   - Activates the relay when an "ON" command is received.
3. **Button Control:**
   - Activates the relay for 10 seconds when the button is pressed.
4. **OLED Display:**
   - Displays the generated TOTP as a QR code.

## Getting Started

### Prerequisites
1. Install the Arduino IDE or ESP32 development environment.
2. Install required libraries:
   - SH1106Wire
   - Base32 Decoder
   - PubSubClient (for MQTT)
   - TOTP

### Uploading the Code
1. Connect your ESP32 to the computer via USB.
2. Open the project in the Arduino IDE.
3. Select the appropriate board and COM port.
4. Compile and upload the code.

### Running the Project
1. Power the ESP32 module.
2. The device will connect to Wi-Fi and synchronize time with an NTP server.
3. A TOTP will be generated every 30 seconds and displayed as a QR code on the OLED screen.
4. Pressing the button or receiving an MQTT "ON" command will activate the relay for 10 seconds.

## Notes
- Ensure that the relay module is correctly powered.
- Use a proper power source for stable OLED operation.
- Keep the secret key secure to maintain the integrity of the TOTP system.

## License
This project is licensed under the MIT License. See the LICENSE file for more details.

---

This README provides a comprehensive guide to set up and run your project. Let me know if you'd like to customize or extend it further!