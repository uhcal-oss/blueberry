# 4006a_dude - Arduino R4 WiFi Fire & Gas Detection System

[![Arduino](https://img.shields.io/badge/Arduino-%2300979D.svg?style=for-the-badge&logo=Arduino&logoColor=white)](https://www.arduino.cc/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge&logo=opensourceinitiative&logoColor=white)](https://opensource.org/licenses/MIT)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/batuh/library/4006a_dude.svg?style=for-the-badge)](https://registry.platformio.org/libraries/batuh/4006a_dude)

## Description

This project implements a fire and gas detection system using the Arduino Uno R4 WiFi board. It monitors flame, MQ2 (flammable gas/smoke), and MQ7 (carbon monoxide) sensors, displays status on the built-in LED matrix, and sends emergency alerts via HTTP to a Google Apps Script endpoint when thresholds are exceeded.

## Features

- Real-time sensor monitoring (Flame, MQ2, MQ7)
- Visual status indication on 8x12 LED matrix:
  - WiFi connection animation
  - Alert flashing pattern
  - Pong game during normal operation
- HTTP emergency alert system with redirect handling
- Manual trigger via serial input ('t' key)
- Configurable thresholds and timing

## Hardware Requirements

- Arduino Uno R4 WiFi
- Flame sensor (analog)
- MQ2 gas/smoke sensor (analog)
- MQ7 carbon monoxide sensor (analog)
- Breadboard and jumper wires

## Wiring

| Sensor | Arduino Pin |
|--------|-------------|
| Flame  | A0          |
| MQ2    | A1          |
| MQ7    | A2          |

*All sensors share common ground and 5V power.*

## Setup

1. Install required libraries via Arduino Library Manager:
   - WiFiS3
   - ArduinoHttpClient
   - Arduino_LED_Matrix

2. Copy your WiFi credentials to the top of `4006a_dude.ino`:
   ```cpp
   const char* SSID = "your_SSID";
   const char* PASS = "your_PASSWORD";
   ```

3. Update the Google Apps Script endpoint:
   ```cpp
   const char* RELAY_HOST = "script.google.com";
   const char* RELAY_PATH = "/macros/s/YOUR_SCRIPT_ID/exec";
   ```

4. Upload the sketch to your Arduino Uno R4 WiFi.

## Usage

- Power on the board
- Watch for WiFi connection animation on LED matrix
- Normal operation displays a Pong game
- When sensor thresholds are exceeded:
  - LED matrix flashes alert pattern
  - Emergency alert is sent via HTTP
  - System returns to monitoring after alert
- Press 't' in Serial Monitor to manually trigger an alert

## How It Works

1. **WiFi Connection**: Board connects to specified network, showing animation on LED matrix
2. **Sensor Monitoring**: Every 2 seconds, reads analog values from all three sensors
3. **Threshold Check**: Compares readings against predefined thresholds:
   - Flame: < 500 (trigger when no flame detected - adjust based on your sensor)
   - MQ2: > 400 (flammable gas/smoke)
   - MQ7: > 400 (carbon monoxide)
4. **Alert Sequence**: On threshold breach:
   - Flashes alert pattern 5 times
   - Sends GET request to Google Apps Script with sensor values
   - Handles 302 redirect to final endpoint
5. **Normal Operation**: Displays interactive Pong game when no alerts

## Customization

- Adjust thresholds in the code:
  ```cpp
  const int THRESHOLD_FLAME = 500;
  const int THRESHOLD_MQ2 = 400;
  const int THRESHOLD_MQ7 = 400;
  ```
- Modify alert duration/frequency in the `loop()` function
- Change Pong game speed by adjusting `lastPongUpdate` interval
- Update LED matrix frames in the frame arrays at the top

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- Arduino Uno R4 WiFi documentation
- Google Apps Script for HTTP endpoint handling
- Open-source sensor libraries and examples

---

*Built with ❤️ for safety and IoT education.*