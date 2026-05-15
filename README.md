# 4006a_dude - Arduino R4 WiFi Fire & Gas Detection System

[![Arduino](https://img.shields.io/badge/Arduino-%2300979D.svg?style=for-the-badge&logo=Arduino&logoColor=white)](https://www.arduino.cc/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge&logo=opensourceinitiative&logoColor=white)](https://opensource.org/licenses/MIT)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/batuh/library/4006a_dude.svg?style=for-the-badge)](https://registry.platformio.org/libraries/batuh/4006a_dude)

## Description

This project implements a fire and gas detection system using the Arduino Uno R4 WiFi board. It monitors flame, MQ2 (flammable gas/smoke), and MQ7 (carbon monoxide) sensors, displays status on the built-in LED matrix, and sends emergency alerts via HTTP to a Google Apps Script endpoint when thresholds are exceeded.

## Features

### Core Detection & Alerts
- Real-time sensor monitoring (Flame, MQ2, MQ8)
- **Multiple alert levels** (WARNING vs CRITICAL with different buzzer patterns)
- **Audible alarm** with 4 distinct buzzer patterns
- Visual status on 8x12 LED matrix
- HTTP emergency alert via Google Apps Script
- BLE fallback when WiFi unavailable

### Data & Storage
- **EEPROM storage** for alert history (stores up to 10 alerts)
- **Timestamp logging** for all events (Real-Time Clock)
- **Last alert details** with geolocation data
- **Alert counter** tracking total events
- **Rate limiting** (5-second cooldown to prevent spam)

### Advanced Features
- **WiFi-based geolocation** using Google Geolocation API
- **Sensor calibration mode** for fine-tuning accuracy
- **Automatic startup diagnostics** with self-check
- **Extended data structure** ready for temperature/humidity sensors
- **Test mode** for safe development/testing
- **Comprehensive serial commands** for full system control
- **Web dashboard** - Real-time monitoring interface (HTTP)
- **CSV data export** - Download alert history
- **Email alerts** - Notifications sent to configured recipient
- **Remote API** - HTTP endpoints for automation
- **Trend analysis** - Predictive warnings for alert patterns

### Configuration
- Centralized configuration in `config.h`
- Secure API key storage
- Customizable sensor thresholds
- Adjustable alert cooldown period
- Modular code structure for easy expansion

## Hardware Requirements

- Arduino Uno R4 WiFi
- Flame sensor module (IR)
- MQ2 gas/smoke sensor (with analog + digital output)
- MQ8 hydrogen sensor (with analog + digital output)
- **Active buzzer** (or passive buzzer + 100Ω-200Ω resistor)
- Breadboard and jumper wires
- USB power (5V) or external 5-12V adapter

## Wiring

| Component | Arduino Pin | Type |
|-----------|-------------|------|
| Flame Sensor | A0 | Analog |
| MQ2 Sensor | A1 | Analog |
| MQ8 Sensor | A2 | Analog |
| MQ2 Digital | Pin 3 | Digital |
| MQ8 Digital | Pin 4 | Digital |
| Buzzer | Pin 5 | Digital Output |

*See [WIRING.md](WIRING.md) for detailed connection diagrams.*

## Setup

1. **Install required libraries** via Arduino Library Manager:
   - `WiFiS3`
   - `ArduinoHttpClient`
   - `Arduino_LED_Matrix`
   - `RTCZero` (for timestamp logging)

2. **Configure credentials** in `config.h`:
   ```cpp
   const char* SSID = "your_SSID";
   const char* PASS = "your_PASSWORD";
   const char* GEOLOCATION_API_KEY = "your_google_api_key";
   const char* RELAY_PATH = "your_google_apps_script_url";
   ```

3. **Adjust sensor thresholds** in `config.h` if needed:
   ```cpp
   const int THRESHOLD_FLAME = 100;   // Adjust for your sensor
   const int THRESHOLD_MQ2 = 700;     // Adjust for your environment
   const int THRESHOLD_MQ8 = 400;     // Adjust for your environment
   ```

4. **Connect hardware**:
   - Wire sensors to analog pins (A0-A2)
   - Connect digital alarm pins (3-4)
   - Connect buzzer to Pin 5
   - Common ground and 5V power to all sensors

5. **Upload the sketch**:
   - Select board: `Arduino UNO R4 WiFi`
   - Select port: Your COM port
   - Click Upload

6. **Open Serial Monitor** (115200 baud) to see diagnostics and control system

## Usage

### Serial Monitor Commands (115200 baud)

**Alert Control:**
- `t` - Manually trigger test alert
- `m` - Toggle test mode ON/OFF (suppresses actual alerts)

**Geolocation:**
- `g` - Get current location via WiFi scanning
- `l` - Display last known location

**Calibration & Diagnostics:**
- `c` - Start/stop sensor calibration mode
- `s` - Run full sensor self-diagnostics
- `d` - Quick system status check

**Data Management:**
- `h` - Display alert history (all stored records)
- `x` - Clear all alert history
- `p` - Print details of last alert
- `a` - Show alert statistics
- `e` - Show extended sensor data capabilities

**Dashboard & Remote Control:**
- `w` - Web dashboard info and status
- `v` - Preview CSV data export
- `n` - Send test email alert
- `r` - Show remote API commands
- `z` - Display alert trend analysis

### Typical Operation Flow

1. **Power On**:
   - Single beep confirms startup
   - Automatic sensor diagnostics run
   - System connects to WiFi
   - LED matrix shows connection status

2. **Normal Monitoring**:
   - Reads sensors every 2 seconds
   - Sends readings to visualization
   - Checks against thresholds

3. **Alert Triggered**:
   - Double beep (WARNING) or rapid beeps (CRITICAL)
   - LED matrix flashes alert pattern
   - Alert sent via WiFi or BLE
   - Alert details stored in EEPROM with timestamp & location
   - Rate limit prevents alert spam (5-second cooldown)

4. **Data Review**:
   - Press `p` to see last alert details (timestamp, location, sensor values)
   - Press `h` to view alert history
   - Press `a` for statistics

5. **Remote Access** (When WiFi Connected):
   - Access web dashboard: `http://YOUR_ARDUINO_IP/`
   - Download alert history: `http://YOUR_ARDUINO_IP/?action=csv`
   - Trigger test alert: `http://YOUR_ARDUINO_IP/?action=test`
   - Email alerts sent automatically on critical events

6. **Trend Detection**:
   - System automatically monitors alert patterns
   - Triggers predictive warning on critical trends (3+ alerts in 60 sec)
   - Long beep pattern indicates trend detection

## Web Dashboard & Remote Features

### Accessing the Dashboard
When WiFi is connected, access the real-time monitoring dashboard:
```
http://<your_arduino_ip>/
```

Features available:
- **Real-time Sensor Display** - Current readings for all sensors
- **System Status** - WiFi, BLE, test mode, alert count
- **Last Alert Details** - Complete info from most recent alert
- **Quick Actions** - Buttons for: Test Alert, Download CSV, Clear History
- **Responsive Design** - Works on desktop, tablet, and mobile

### Remote API Endpoints

```
GET /                    → Dashboard HTML interface
GET /?action=csv         → Download alert history as CSV
GET /?action=test        → Trigger test alert
GET /?action=clear       → Clear all alert history
```

### Email Alerts
Configure email recipient in `config.h`:
```cpp
const char* EMAIL_RECIPIENT = "your-email@gmail.com";
```

When configured, critical alerts are automatically emailed with:
- Alert level (WARNING/CRITICAL)
- Sensor readings
- Geolocation coordinates
- Timestamp

Send test email with serial command: `n`

### Trend Analysis
System performs automatic trend analysis:
- Monitors alert frequency
- Detects patterns indicating cascading failures
- Issues predictive warnings
- Configure sensitivity in `config.h`

View trend status: Serial command `z`

## How It Works

1. **Startup Sequence**:
   - Initializes sensors, storage (EEPROM), and Real-Time Clock
   - Runs automatic diagnostics
   - Connects to WiFi (or enables BLE as fallback)

2. **Continuous Monitoring** (Every 2 seconds):
   - Reads analog values from flame, MQ2, and MQ8 sensors
   - Reads digital alarm pins
   - Updates LED matrix visualization

3. **Threshold Evaluation**:
   - Compares sensor readings against configured thresholds
   - Determines alert level:
     - **WARNING**: Minor threshold breach
     - **CRITICAL**: Major threshold breach
   - Checks rate limit to prevent alert spam

4. **Alert Triggered**:
   - Gets geolocation (if WiFi connected)
   - Logs alert to EEPROM with timestamp
   - Stores alert details (sensor values, location, alert level)
   - Plays appropriate buzzer pattern
   - Sends emergency notification via HTTP or BLE
   - Updates alert counter

5. **Data Storage**:
   - Maintains EEPROM history of up to 10 alerts
   - Stores timestamp, sensor values for each alert
   - Available for review via serial commands

6. **Calibration Mode** (Optional):
   - Allows fine-tuning sensor thresholds
   - Captures min/max flame sensor readings
   - Helps adapt to different environments

## Customization

All configurable settings are in **`config.h`**:

### Sensor Thresholds
```cpp
const int THRESHOLD_FLAME = 100;    // Flame detection threshold
const int THRESHOLD_MQ2 = 700;      // Gas/smoke threshold
const int THRESHOLD_MQ8 = 400;      // Hydrogen threshold
const int FLAME_CRITICAL_THRESHOLD = 100;  // Critical level
```

### Alert Settings
```cpp
const unsigned long ALERT_COOLDOWN_MS = 5000;  // Minimum between alerts
const int MAX_ALERT_HISTORY = 10;              // EEPROM history size
```

### Buzzer Patterns (timing in ms)
```cpp
const int BUZZER_BEEP_SHORT = 100;   // Quick beep
const int BUZZER_BEEP_LONG = 500;    // Long beep
const int BUZZER_SILENCE = 200;      // Pause between beeps
```

### API Configuration
```cpp
const char* SSID = "your_network";
const char* PASS = "your_password";
const char* GEOLOCATION_API_KEY = "your_google_key";
```

### Extending the System
- Add temperature/humidity: Connect DHT22 to Pin 7
- Add new gas sensors: Use additional analog pins
- Modify buzzer patterns: Edit `playBuzzerPattern()` function
- Custom geolocation: Modify `getGeolocation()` function

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgements

- Arduino Uno R4 WiFi documentation
- Google Apps Script for HTTP endpoint handling
- Open-source sensor libraries and examples

---

*Built with ❤️ for safety and IoT education.*