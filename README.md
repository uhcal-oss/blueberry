<div align="center">

# Blueberry Tubitak

**Autonomous Forest Fire Early Warning System**

Built with Arduino + Firebase Cloud Messaging + Jetpack Compose

---

![Android](https://img.shields.io/badge/Android-SDK_26+-34A853?style=for-the-badge&logo=android&logoColor=white)
![Kotlin](https://img.shields.io/badge/Kotlin-2.2-7F52FF?style=for-the-badge&logo=kotlin&logoColor=white)
![Jetpack Compose](https://img.shields.io/badge/Jetpack_Compose-Material3-4285F4?style=for-the-badge&logo=jetpackcompose&logoColor=white)
![Firebase](https://img.shields.io/badge/Firebase-FCM-FFCA28?style=for-the-badge&logo=firebase&logoColor=black)
![Arduino](https://img.shields.io/badge/Arduino-UNO_R4_WiFi-00878F?style=for-the-badge&logo=arduino&logoColor=white)

</div>

---

## Overview

Blueberry is an autonomous forest fire detection and alerting system developed for TUBiTAK. It consists of three components:

1. **Field Station** — Arduino UNO R4 WiFi with flame (A0), smoke MQ-2 (A1), and carbon monoxide MQ-7 (A2) sensors
2. **Cloud Relay** — Google Apps Script that receives sensor data and dispatches Firebase Cloud Messages
3. **Mobile App** — Android application that receives alerts, provides a monitoring dashboard, and can autonomously notify emergency services via SMS

When sensor thresholds are breached, the Arduino fires an HTTP request through Google Apps Script, which triggers an FCM push notification to the companion app. The app gives the user a 10-60 second countdown to intervene before automatically sending an SMS with coordinates and telemetry to the configured emergency number.

---

## Architecture

```
┌──────────────┐       HTTPS        ┌──────────────────┐       FCM        ┌──────────────┐
│   Arduino    │ ──────────────────> │ Google Apps      │ ───────────────> │  Android     │
│   Sensors    │   sensor params     │ Script (relay)   │   push + data    │  App         │
└──────────────┘                     └──────────────────┘                  └──────┬───────┘
                                                                                  │
                                                                           SMS to 112
                                                                          (autonomous)
```

---

## Features

### Mobile App
- Material 3 + Material You dynamic theming
- Bottom navigation with Dashboard, Alerts, and Settings tabs
- Real-time sensor gauge indicators with threshold visualization
- Emergency action screen with countdown timer and red gradient UI
- Swipe-to-dismiss alert management with Room-free persistent storage
- Dark/Light mode toggle
- Configurable countdown timer (10/20/30/60 seconds)
- Map integration — tap coordinates to open in Google Maps
- Alert severity color coding (Critical / Warning)
- Animated connection status indicators
- Splash screen with branded theme
- Edge-to-edge display
- Inter font via Google Fonts
- Turkish localization throughout

### Field Station (Arduino)
- Flame sensor (analog, A0)
- MQ-2 smoke/gas sensor (analog A1, digital D3)
- MQ-7 carbon monoxide sensor (analog A2, digital D4)
- WiFi primary / BLE fallback connectivity
- Google Geolocation API for coordinate detection
- EEPROM alert history logging
- Built-in web dashboard (port 80)
- LED matrix visualization
- Buzzer alert patterns by severity
- Serial command interface for diagnostics
- Rate-limited alerting with cooldown

### Cloud Relay (Google Apps Script)
- JWT-authenticated FCM v1 API calls
- Passes sensor values, coordinates, and severity to mobile app
- Supports both GET and POST from Arduino

---

## Setup

### Prerequisites
- Android Studio (latest stable)
- Arduino IDE with Arduino UNO R4 WiFi board support
- A Firebase project with Cloud Messaging enabled
- Google Apps Script deployment

### Android App
1. Clone this repository
2. Open in Android Studio
3. Place your `google-services.json` from the Firebase Console into `/app/`
4. Build and run on a physical device (FCM requires Google Play Services)

### Arduino
1. Open the `.ino` sketch in Arduino IDE
2. Create a `config.h` with your WiFi credentials, relay host, thresholds, and API keys
3. Upload to Arduino UNO R4 WiFi

### Google Apps Script
1. Create a new Apps Script project
2. Paste the `Code.gs` contents
3. Set your Firebase service account credentials in Script Properties
4. Deploy as a web app (Execute as: Me, Access: Anyone)

---

## Project Structure

```
app/src/main/java/com/batuhantrkgl/blueberrytubitak/
├── MainActivity.kt              # Dashboard, Alerts, Settings UI
├── EmergencyActionActivity.kt   # Emergency countdown + SMS dispatch
├── EmergencyAlertManager.kt     # Persistent alert storage (SharedPrefs + Gson)
├── EmergencyMessagingService.kt # FCM message handler
├── EmergencyTTSManager.kt       # Text-to-speech for autonomous calls
├── EmergencyChannel.kt          # Notification channel setup
├── AlertEntity.kt               # Alert data model
├── MainViewModel.kt             # ViewModel for alert state
└── ui/theme/
    ├── Color.kt                 # Brand colors
    ├── Theme.kt                 # Material 3 theming
    └── Type.kt                  # Inter font typography
```

---

## Tech Stack

| Layer | Technology |
|-------|-----------|
| UI | Jetpack Compose, Material 3, Material You |
| State | ViewModel, StateFlow |
| Persistence | SharedPreferences + Gson |
| Push | Firebase Cloud Messaging (v1 API) |
| Fonts | Google Fonts Provider (Inter) |
| Icons | Compose Tabler Icons |
| Relay | Google Apps Script |
| Hardware | Arduino UNO R4 WiFi, MQ-2, MQ-7, Flame Sensor |

---

## License

This project was developed as part of a TUBiTAK research submission.

---

<div align="center">
<sub>Blueberry-1 Station — Izmir, Turkey</sub>
</div>
