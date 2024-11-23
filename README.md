# README for ESP32 Door Lock System

## Overview

This project implements a secure door lock system using an ESP32 microcontroller. It combines a Time-based One-Time Password (TOTP) generator, a web interface, and physical controls with a relay and button. The project features a QR code display for TOTP codes, a responsive web server, and a button to control a relay for door locking/unlocking.

---

## Features

1. **TOTP Generation:**
   - Generates a new TOTP every 30 seconds using a Base32-encoded secret key.
   - Displays the TOTP as a QR code on an SH1106 OLED display.

2. **Web Server:**
   - Simple HTTP server running on the ESP32.
   - Displays a basic webpage accessible at the ESP32's local IP.

3. **Relay and Button Control:**
   - A button controls a relay that activates for 10 seconds when pressed.
   - The OLED display turns off while the relay is active and resumes afterward.

4. **Wi-Fi Connectivity:**
   - Connects to a predefined Wi-Fi network.
   - Synchronizes the system time with an NTP server.

5. **Concurrency with FreeRTOS:**
   - Three tasks run concurrently for TOTP generation, web server handling, and button/relay control.
   - Uses FreeRTOS mechanisms like task prioritization and mutexes.

---

## Hardware Requirements

1. **ESP32 Dev Board**
2. **SH1106 OLED Display**
3. **Relay Module**
4. **Push Button**
5. **Wi-Fi Network**

---

## Software Requirements

1. **Arduino IDE** with ESP32.
2. **Libraries:**
   - `Base32-Decode`
   - `Wire`
   - `SH1106Wire`
   - `qrcodeoled`
   - `TOTP`
   - `WiFi`
   - `NetworkClient`
   - `WebServer`
   - `time`

---

## Pin Configuration

| Component     | Pin            |
|---------------|----------------|
| OLED (SDA)    | `GPIO21`        |
| OLED (SCL)    | `GPIO22`       |
| Relay         | `GPIO5`        |
| Button        | `GPIO17`       |

---

## Code Explanation

### 1. **TOTP Generation**
- A Base32-encoded secret key is decoded during setup.
- The decoded key is used to initialize the `TOTP` library.
- A FreeRTOS task (`totpTask`) generates a new TOTP every 30 seconds.
- The TOTP is displayed on the OLED as a QR code.

### 2. **Web Server**
- A simple web server runs on port 80.
- Responds to HTTP GET requests at `/` with a basic HTML page.
- Managed in a separate FreeRTOS task (`webServerTask`).

### 3. **Relay and Button Control**
- A push button activates the relay for 10 seconds.
- While the relay is active, the OLED display turns off.
- After the relay deactivates, the OLED display resumes.

### 4. **FreeRTOS Tasks**
- **`totpTask`**: Handles TOTP generation and OLED updates.
- **`webServerTask`**: Manages HTTP server requests.
- **`buttonRelayTask`**: Handles button presses and relay control.

### 5. **Wi-Fi Connectivity**
- The ESP32 connects to the predefined SSID and password.
- Synchronizes time using the `configTime` function.

---

## How to Use

1. **Setup Hardware:**
   - Connect the components as per the pin configuration.

2. **Upload Code:**
   - Install the required libraries.
   - Upload the code to the ESP32 using Arduino IDE or ESP-IDF.

3. **Run the System:**
   - Power the ESP32.
   - Connect to the Wi-Fi network.

4. **Access Features:**
   - View the TOTP QR code on the OLED.
   - Access the web server by entering the ESP32's IP address in a browser.
   - Press the button to activate the relay.

---

## Customization

1. **Wi-Fi Credentials:**
   - Modify `ssid` and `password` in the code to match your network.

2. **TOTP Secret Key:**
   - Replace the `secretKey` with your own Base32-encoded key.

3. **Timing:**
   - Adjust the relay activation time by modifying the delay in `buttonRelayTask`.

---

## Example Outputs

1. **Serial Monitor:**
   - Displays Wi-Fi connection status, generated TOTPs, and timestamps.

2. **OLED Display:**
   - Shows a QR code representing the current TOTP.

3. **Web Server:**
   - Accessible via the ESP32's IP address, displaying a "Hello ESP32" message.

---

## Troubleshooting

- **Wi-Fi Not Connecting:**
  - Ensure correct SSID and password.
  - Verify signal strength.

- **No TOTP Display:**
  - Check the OLED connections.
  - Ensure the secret key is correctly encoded in Base32.

- **Button Not Working:**
  - Verify the button and relay connections.
  - Check for debounce issues.

---

This README provides a comprehensive guide to the ESP32 Door Lock System, ensuring ease of setup and operation.