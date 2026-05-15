# Arduino R4 WiFi - Fire/Gas Detection System Wiring Guide

## Pin Configuration

| Component | Arduino Pin | Type | Purpose |
|-----------|-------------|------|---------|
| Flame Sensor (Analog) | A0 | Analog Input | IR flame detection |
| MQ2 Sensor (Analog) | A1 | Analog Input | Smoke/Gas detection |
| MQ8 Sensor (Analog) | A2 | Analog Input | Hydrogen detection |
| MQ2 Digital Output | Pin 3 | Digital Input | MQ2 alarm pin |
| MQ8 Digital Output | Pin 4 | Digital Input | MQ8 alarm pin |
| **Buzzer/Alarm** | **Pin 5** | **Digital Output** | **Audio Alert** |
| LED Matrix | I2C (SDA, SCL) | I2C | LED Display |
| WiFi/BLE | Built-in | Wireless | Communication |

---

## Connection Diagrams

### Sensor Connections

```
FLAME SENSOR (IR Receiver Module)
├── VCC ──────────→ 5V (Arduino)
├── GND ──────────→ GND (Arduino)
└── A0 ──────────→ Pin A0 (Analog)

MQ2 SENSOR (Smoke/Gas)
├── VCC ──────────→ 5V (Arduino)
├── GND ──────────→ GND (Arduino)
├── AO ──────────→ Pin A1 (Analog)
└── D0 ──────────→ Pin 3 (Digital)

MQ8 SENSOR (Hydrogen)
├── VCC ──────────→ 5V (Arduino)
├── GND ──────────→ GND (Arduino)
├── AO ──────────→ Pin A2 (Analog)
└── D0 ──────────→ Pin 4 (Digital)
```

### Buzzer Connection

```
ACTIVE BUZZER (or Passive Buzzer + Resistor)
├── Positive (+) ─→ Pin 5 (Digital Output)
└── Negative (-) ─→ GND (Arduino)

NOTE: If using passive buzzer, add 100Ω-200Ω resistor between Pin 5 and Buzzer+
```

### LED Matrix (Built-in)

```
ARDUINO R4 BUILT-IN COMPONENTS
├── LED Matrix ──→ No wiring needed (built-in I2C)
├── WiFi ────────→ No wiring needed (built-in module)
└── BLE ─────────→ No wiring needed (built-in module)
```

---

## Power Connections Summary

### All Sensors
```
5V (Arduino) ────→ All VCC pins
GND (Arduino) ───→ All GND pins
```

### Complete Wiring Layout

```
                    ARDUINO R4 WIFI
                  ┌──────────────────┐
                  │                  │
            ┌─────┤ 5V           GND ├─────┬─────────┐
            │     │                  │     │         │
            │     │ A0 A1 A2        │     │         │
            │     │ │  │  │          │     │         │
            │     └──────────────────┘     │         │
            │         │  │  │              │         │
            │    ┌────┴──┴──┴──┐          │         │
            │    │ SENSORS     │          │         │
            │    │ Flame/MQ2/MQ8          │         │
            │    └────┬──┬──┬──┘          │         │
            │         │  │  │             │         │
            │         └──┴──┴─────────────┼─────────┘
            │                             │
            ├─→ Pin 3 (MQ2 Digital)      │
            ├─→ Pin 4 (MQ8 Digital)      │
            └─→ Pin 5 (BUZZER) ←─────────┘
                    │
                    │
              ┌─────▼─────┐
              │   BUZZER   │
              │ 5V / GND   │
              └────────────┘
```

---

## Sensor Signal Levels

### Analog Sensors (A0, A1, A2)
- **Input Range**: 0V - 3.3V (mapped to 0-1023)
- **Flame Sensor**: Active LOW when flame detected
- **MQ2/MQ8**: Return analog values proportional to gas concentration

### Digital Sensors (Pin 3, Pin 4)
- **Logic Level**: HIGH (3.3V) when triggered
- **Default**: LOW when no gas detected
- **Used for**: Quick detection without analog reading

### Buzzer (Pin 5)
- **Output**: 3.3V HIGH = Buzzer ON
- **Output**: LOW = Buzzer OFF
- **Current**: ~20-100mA depending on buzzer type

---

## Hardware Requirements

### Essential Components
- Arduino R4 WiFi
- Flame Sensor Module (IR)
- MQ2 Gas Sensor
- MQ8 Hydrogen Sensor
- Active Buzzer or Passive Buzzer (+ resistor)
- Jumper Wires (male-to-male, male-to-female)
- Breadboard (optional, for cleaner wiring)

### Power Supply
- **USB Power**: 5V from USB cable (for development)
- **External Power**: 5-12V adapter recommended for deployment
- **Current Draw**: ~500mA typical (WiFi active)

---

## ⚡ Assembly Steps

1. **Connect Sensors**
   - Attach VCC from all sensors to Arduino 5V
   - Attach GND from all sensors to Arduino GND
   - Attach Flame AO to Pin A0
   - Attach MQ2 AO to Pin A1, D0 to Pin 3
   - Attach MQ8 AO to Pin A2, D0 to Pin 4

2. **Connect Buzzer**
   - Positive pin → Arduino Pin 5
   - Negative pin → Arduino GND

3. **USB Connection**
   - Connect Arduino to computer via USB-C cable
   - Select board: Tools → Board → Arduino UNO R4 WiFi
   - Select port: Tools → Port → COMx

4. **Upload Code**
   - Open `4006a_dude.ino`
   - Click Upload
   - Serial output visible in Tools → Serial Monitor (115200 baud)

---

## Testing & Command Reference

### All Serial Monitor Commands (115200 baud)

| Key | Function | Category |
|-----|----------|----------|
| **t** | Trigger test alert | Alert Control |
| **m** | Toggle test mode ON/OFF | Alert Control |
| **g** | Get geolocation via WiFi | Geolocation |
| **l** | Show current location | Geolocation |
| **c** | Start/stop calibration mode | Calibration |
| **h** | Show alert history (all records) | Data Review |
| **x** | Clear alert history | Data Cleanup |
| **d** | Quick system status | Diagnostics |
| **s** | Full sensor diagnostics | Diagnostics |
| **p** | Print last alert details | Data Review |
| **a** | Show alert statistics | Data Review |
| **e** | Extended sensor data info | Data Review |
| **w** | Web dashboard info | Dashboard |
| **v** | CSV data export preview | Data Export |
| **n** | Send test email alert | Remote Control |
| **r** | Remote API commands | Remote Control |
| **z** | Alert trend analysis | Predictive |

### Expected Behavior

| Action | Expected Output |
|--------|-----------------|
| Power on | Single beep + Full diagnostics output |
| Send 't' | Alert triggered with pattern based on severity |
| Send 's' | Complete sensor self-check with pass/fail |
| Send 'c' | Start calibration mode, captures flame min/max |
| Send 'p' | Shows last alert with timestamp & location |
| Flame detected | Double beep (WARNING) or rapid beep (CRITICAL) |
| High gas (MQ2/MQ8) | Critical alert triggered |

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| **No buzzer sound** | Check Pin 5 connection & buzzer polarity |
| **Sensor reading 1023** | Check VCC/GND connections |
| **Sensor reading 0** | Check A0/A1/A2 connections |
| **No Serial output** | Check USB cable, select correct COM port, 115200 baud |
| **WiFi won't connect** | Check SSID/password in config.h |

---

## Notes

- **Flame Sensor**: Takes 1-2 seconds to stabilize after power-on
- **MQ Gas Sensors**: Need 24-48 hours warm-up for best accuracy
- **Buzzer Polarity**: Active buzzers have a + and - side (don't reverse!)
- **I2C Conflict**: LED Matrix uses I2C (built-in, no external wiring needed)
- **API Key**: Update `config.h` with your Google Geolocation API key

---

Generated: May 15, 2026
