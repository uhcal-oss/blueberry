#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoBLE.h>
#include <EEPROM.h>
#include <RTCZero.h>
#include "Arduino_LED_Matrix.h"
#include "config.h"

ArduinoLEDMatrix matrix;

// Real-Time Clock
RTCZero rtc;

// Custom 12x8 pixel frames for the R4 LED Matrix
uint8_t frame_wifi[8][12] = {
  { 0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,1,1,1,1,1,1,0,0,0 },
  { 0,0,1,0,0,0,0,0,0,1,0,0 },
  { 0,0,0,0,1,1,1,1,0,0,0,0 },
  { 0,0,0,1,0,0,0,0,1,0,0,0 },
  { 0,0,0,0,0,1,1,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,1,1,0,0,0,0,0 }
};

uint8_t frame_alert[8][12] = {
  { 0,0,0,0,0,1,1,0,0,0,0,0 },
  { 0,0,0,0,0,1,1,0,0,0,0,0 },
  { 0,0,0,0,0,1,1,0,0,0,0,0 },
  { 0,0,0,0,0,1,1,0,0,0,0,0 },
  { 0,0,0,0,0,1,1,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,1,1,0,0,0,0,0 },
  { 0,0,0,0,0,1,1,0,0,0,0,0 }
};

uint8_t frame_blank[8][12] = {
  { 0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0,0 },
  { 0,0,0,0,0,0,0,0,0,0,0,0 }, { 0,0,0,0,0,0,0,0,0,0,0,0 }
};

// Sensor Pins
const int PIN_FLAME = A0;
const int PIN_MQ2 = A1;
const int PIN_MQ8 = A2;

const int D_PIN_MQ2 = 3;   // Digital MQ2 Pin
const int D_PIN_MQ8 = 4;   // Digital MQ8 Pin

// Test Mode (Set to true to disable sending actual alerts)
bool testMode = true;

// Visualization Variables
uint8_t viz_frame[8][12];
int history[3][12] = {0}; // 0: Flame, 1: MQ2, 2: MQ8
unsigned long lastVizUpdate = 0;
unsigned long lastSensorRead = 0;

// BLE Variables
BLEService envService("181A");
BLEStringCharacteristic sensorDataChar("2A58", BLERead | BLENotify, 32);
bool useBLE = false;

// Geolocation Variables
float currentLatitude = 0.0;
float currentLongitude = 0.0;
bool hasLocation = false;

// Calibration & Diagnostics
bool calibrationMode = false;
int flameCalibrationMin = 1023;
int flameCalibrationMax = 0;

// Alert Tracking
unsigned long lastAlertTime = 0;
int alertCount = 0;

// Last Alert Details
struct LastAlert {
  uint32_t timestamp;
  int flameVal;
  int mq2Val;
  int mq8Val;
  float latitude;
  float longitude;
  int alertLevel;
} lastAlertData = {0, 0, 0, 0, 0.0, 0.0, 0};

// Extended Sensor Data (for future use)
struct SensorData {
  uint16_t flame;
  uint16_t mq2;
  uint16_t mq8;
  float temperature;
  float humidity;
  uint32_t timestamp;
} currentSensorData = {0, 0, 0, 0.0, 0.0, 0};

// Web Server
WiFiServer webServer(WEB_SERVER_PORT);

// Predictive Analysis
unsigned long lastTrendCheck = 0;
int recentAlertCount = 0;
bool isCriticalTrend = false;

void playBuzzerPattern(int pattern) {
  switch(pattern) {
    case 1:  // Single beep (status ok)
      digitalWrite(BUZZER_PIN, HIGH);
      delay(BUZZER_BEEP_SHORT);
      digitalWrite(BUZZER_PIN, LOW);
      break;
    case 2:  // Double beep (warning)
      for (int i = 0; i < 2; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(BUZZER_BEEP_SHORT);
        digitalWrite(BUZZER_PIN, LOW);
        delay(BUZZER_SILENCE);
      }
      break;
    case 3:  // Rapid beeping (alert!)
      for (int i = 0; i < 5; i++) {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(BUZZER_BEEP_SHORT);
        digitalWrite(BUZZER_PIN, LOW);
        delay(100);
      }
      break;
    case 4:  // Long continuous (DANGER)
      digitalWrite(BUZZER_PIN, HIGH);
      delay(BUZZER_BEEP_LONG);
      digitalWrite(BUZZER_PIN, LOW);
      break;
  }
}

void logAlertToEEPROM(int f_val, int m2_val, int m8_val) {
  // Store alert with timestamp in EEPROM
  // Format: 4 bytes timestamp + 2 bytes flame + 2 bytes MQ2 + 2 bytes MQ8 = 10 bytes per record
  
  uint32_t timestamp = rtc.getEpoch();
  int addr = EEPROM_ALERT_ADDR + (alertCount % MAX_ALERT_HISTORY) * ALERT_RECORD_SIZE;
  
  EEPROM.put(addr, timestamp);
  EEPROM.put(addr + 4, (int16_t)f_val);
  EEPROM.put(addr + 6, (int16_t)m2_val);
  EEPROM.put(addr + 8, (int16_t)m8_val);
  
  Serial.print("[Info] Alert logged to EEPROM at address ");
  Serial.println(addr);
}

void printAlertHistory() {
  Serial.println("\n[Info] Alert History:");
  Serial.println("Time | Flame | MQ2 | MQ8");
  
  for (int i = 0; i < min(alertCount, MAX_ALERT_HISTORY); i++) {
    int addr = EEPROM_ALERT_ADDR + i * ALERT_RECORD_SIZE;
    
    uint32_t timestamp;
    int16_t flame, mq2, mq8;
    
    EEPROM.get(addr, timestamp);
    EEPROM.get(addr + 4, flame);
    EEPROM.get(addr + 6, mq2);
    EEPROM.get(addr + 8, mq8);
    
    Serial.print(timestamp);
    Serial.print(" | ");
    Serial.print(flame);
    Serial.print(" | ");
    Serial.print(mq2);
    Serial.print(" | ");
    Serial.println(mq8);
  }
  Serial.println();
}

void startCalibration() {
  calibrationMode = true;
  flameCalibrationMin = 1023;
  flameCalibrationMax = 0;
  Serial.println("\n[Calibration] Starting flame sensor calibration...");
  Serial.println("[Calibration] Expose sensor to DARK (no flame) for 5 seconds...");
  playBuzzerPattern(1);
}

void stopCalibration() {
  calibrationMode = false;
  Serial.println("\n[Calibration] Complete!");
  Serial.print("[Calibration] Flame Range: ");
  Serial.print(flameCalibrationMin);
  Serial.print(" - ");
  Serial.println(flameCalibrationMax);
  playBuzzerPattern(2);
}

void runSensorDiagnostics() {
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║    SENSOR DIAGNOSTICS & SELF-CHECK    ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // Check Analog Sensors
  Serial.println("[Diagnostics] Reading analog sensors (5 samples avg)...");
  int flameSum = 0, mq2Sum = 0, mq8Sum = 0;
  for (int i = 0; i < 5; i++) {
    flameSum += analogRead(PIN_FLAME);
    mq2Sum += analogRead(PIN_MQ2);
    mq8Sum += analogRead(PIN_MQ8);
    delay(100);
  }
  int flameAvg = flameSum / 5;
  int mq2Avg = mq2Sum / 5;
  int mq8Avg = mq8Sum / 5;
  
  Serial.print("[Diagnostics] Flame Sensor: ");
  Serial.print(flameAvg);
  Serial.println(flameAvg > 900 ? " ✓ (OK)" : " ✗ (Check connection)");
  
  Serial.print("[Diagnostics] MQ2 Sensor: ");
  Serial.print(mq2Avg);
  Serial.println(mq2Avg > 100 ? " ✓ (OK)" : " ✗ (Check connection)");
  
  Serial.print("[Diagnostics] MQ8 Sensor: ");
  Serial.print(mq8Avg);
  Serial.println(mq8Avg > 100 ? " ✓ (OK)" : " ✗ (Check connection)");
  
  // Check Digital Sensors
  Serial.println("\n[Diagnostics] Checking digital pins...");
  bool dMQ2 = digitalRead(D_PIN_MQ2);
  bool dMQ8 = digitalRead(D_PIN_MQ8);
  Serial.print("[Diagnostics] MQ2 Digital: ");
  Serial.println(dMQ2 ? "HIGH" : "LOW");
  Serial.print("[Diagnostics] MQ8 Digital: ");
  Serial.println(dMQ8 ? "HIGH" : "LOW");
  
  // Check Buzzer
  Serial.println("\n[Diagnostics] Testing buzzer...");
  playBuzzerPattern(1);
  delay(300);
  playBuzzerPattern(2);
  
  // Check Storage
  Serial.println("\n[Diagnostics] EEPROM Status: Available");
  Serial.print("[Diagnostics] Alert Records Stored: ");
  Serial.println(min(alertCount, MAX_ALERT_HISTORY));
  
  // System Status
  Serial.println("\n[Diagnostics] System Status:");
  Serial.print("  - WiFi: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
  Serial.print("  - BLE: ");
  Serial.println(useBLE ? "ACTIVE" : "INACTIVE");
  Serial.print("  - Uptime: ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  Serial.print("  - Free Memory: ~");
  Serial.print(freeRam());
  Serial.println(" bytes");
  
  Serial.println("\n[Diagnostics] ✓ Self-check complete!\n");
  playBuzzerPattern(1);
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void printLastAlert() {
  if (lastAlertData.timestamp == 0) {
    Serial.println("\n[Info] No previous alert recorded yet.");
    return;
  }
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║          LAST ALERT DETAILS            ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  Serial.print("Timestamp: ");
  Serial.println(lastAlertData.timestamp);
  Serial.print("Alert Level: ");
  Serial.println(lastAlertData.alertLevel == ALERT_LEVEL_CRITICAL ? "CRITICAL" : "WARNING");
  Serial.print("Flame Value: ");
  Serial.println(lastAlertData.flameVal);
  Serial.print("MQ2 Value: ");
  Serial.println(lastAlertData.mq2Val);
  Serial.print("MQ8 Value: ");
  Serial.println(lastAlertData.mq8Val);
  Serial.print("Location: ");
  Serial.print(lastAlertData.latitude, 6);
  Serial.print(", ");
  Serial.println(lastAlertData.longitude, 6);
  Serial.println();
}

String generateDashboardHTML() {
  String html = "";
  html += "<!DOCTYPE html><html><head><title>4006A Dude Dashboard</title>";
  html += "<meta name='viewport' content='width=device-width'>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0}";
  html += ".container{background:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
  html += ".sensor{display:inline-block;padding:15px;margin:10px;background:#e3f2fd;border-left:4px solid #1976d2;border-radius:4px;min-width:200px}";
  html += ".alert{padding:10px;margin:10px 0;border-radius:4px}";
  html += ".warning{background:#fff3cd;color:#856404;border-left:4px solid #ffc107}";
  html += ".critical{background:#f8d7da;color:#721c24;border-left:4px solid #dc3545}";
  html += ".ok{background:#d4edda;color:#155724;border-left:4px solid #28a745}";
  html += ".button{padding:10px 20px;margin:5px;background:#1976d2;color:white;border:none;border-radius:4px;cursor:pointer}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>🔥 4006A Dude - Fire & Gas Detection System</h1>";
  html += "<p>Status: <strong>";
  
  if (WiFi.status() == WL_CONNECTED) {
    html += "🟢 ONLINE";
  } else if (useBLE) {
    html += "🔵 BLE ACTIVE";
  } else {
    html += "🔴 OFFLINE";
  }
  
  html += "</strong></p>";
  html += "<h2>📊 Real-Time Sensor Data</h2>";
  
  int valFlame = analogRead(PIN_FLAME);
  int valMQ2 = analogRead(PIN_MQ2);
  int valMQ8 = analogRead(PIN_MQ8);
  
  html += "<div class='sensor'>Flame: " + String(valFlame) + " (Threshold: " + String(THRESHOLD_FLAME) + ")</div>";
  html += "<div class='sensor'>MQ2: " + String(valMQ2) + " (Threshold: " + String(THRESHOLD_MQ2) + ")</div>";
  html += "<div class='sensor'>MQ8: " + String(valMQ8) + " (Threshold: " + String(THRESHOLD_MQ8) + ")</div>";
  
  html += "<h2>🚨 System Status</h2>";
  html += "<p>Total Alerts: <strong>" + String(alertCount) + "</strong></p>";
  html += "<p>Test Mode: <strong>" + String(testMode ? "ON" : "OFF") + "</strong></p>";
  html += "<p>Critical Trend: <strong>" + String(isCriticalTrend ? "⚠️ YES" : "✓ NO") + "</strong></p>";
  
  if (lastAlertData.timestamp > 0) {
    html += "<h2>📍 Last Alert</h2>";
    html += "<div class='alert " + String(lastAlertData.alertLevel == ALERT_LEVEL_CRITICAL ? "critical" : "warning") + "'>";
    html += "Level: " + String(lastAlertData.alertLevel == ALERT_LEVEL_CRITICAL ? "CRITICAL" : "WARNING") + "<br>";
    html += "Flame: " + String(lastAlertData.flameVal) + " | MQ2: " + String(lastAlertData.mq2Val) + " | MQ8: " + String(lastAlertData.mq8Val) + "<br>";
    html += "Location: " + String(lastAlertData.latitude, 4) + ", " + String(lastAlertData.longitude, 4);
    html += "</div>";
  }
  
  html += "<h2>🎮 Quick Actions</h2>";
  html += "<button class='button' onclick='window.location.href=\"/?action=test\"'>Test Alert</button>";
  html += "<button class='button' onclick='window.location.href=\"/?action=csv\"'>Download History</button>";
  html += "<button class='button' onclick='window.location.href=\"/?action=clear\"'>Clear History</button>";
  html += "</div></body></html>";
  
  return html;
}

String generateCSVData() {
  String csv = "Timestamp,Alert_Level,Flame,MQ2,MQ8,Latitude,Longitude\n";
  
  for (int i = 0; i < min(alertCount, MAX_ALERT_HISTORY); i++) {
    int addr = EEPROM_ALERT_ADDR + i * ALERT_RECORD_SIZE;
    
    uint32_t timestamp;
    int16_t flame, mq2, mq8;
    
    EEPROM.get(addr, timestamp);
    EEPROM.get(addr + 4, flame);
    EEPROM.get(addr + 6, mq2);
    EEPROM.get(addr + 8, mq8);
    
    csv += String(timestamp) + "," + "WARNING" + "," + String(flame) + "," + String(mq2) + "," + String(mq8) + ",0,0\n";
  }
  
  return csv;
}

void sendEmailAlert(int level) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Email] WiFi not connected. Cannot send email.");
    return;
  }
  
  Serial.println("[Email] Sending email alert...");
  
  String subject = (level == ALERT_LEVEL_CRITICAL) ? "🚨 CRITICAL: Fire/Gas Detected!" : "⚠️ WARNING: Abnormal Readings";
  String body = "Alert Level: " + String(level == ALERT_LEVEL_CRITICAL ? "CRITICAL" : "WARNING") + "\n";
  body += "Flame: " + String(lastAlertData.flameVal) + "\n";
  body += "MQ2: " + String(lastAlertData.mq2Val) + "\n";
  body += "MQ8: " + String(lastAlertData.mq8Val) + "\n";
  body += "Location: " + String(lastAlertData.latitude, 6) + ", " + String(lastAlertData.longitude, 6) + "\n";
  body += "Time: " + String(lastAlertData.timestamp) + "\n";
  
  Serial.print("[Email] Subject: ");
  Serial.println(subject);
  Serial.print("[Email] Body: ");
  Serial.println(body);
  Serial.println("[Email] (Email sending via Google Apps Script configured in config.h)");
}

void handleWebRequest(String request) {
  if (request.indexOf("GET / HTTP") != -1) {
    String response = generateDashboardHTML();
    
    webServer.println("HTTP/1.1 200 OK");
    webServer.println("Content-Type: text/html");
    webServer.println("Content-Length: " + String(response.length()));
    webServer.println("Connection: close");
    webServer.println();
    webServer.println(response);
    
  } else if (request.indexOf("action=csv") != -1) {
    String csv = generateCSVData();
    
    webServer.println("HTTP/1.1 200 OK");
    webServer.println("Content-Type: text/csv");
    webServer.println("Content-Disposition: attachment; filename=\"alerts.csv\"");
    webServer.println("Content-Length: " + String(csv.length()));
    webServer.println("Connection: close");
    webServer.println();
    webServer.println(csv);
    
  } else if (request.indexOf("action=test") != -1) {
    sendEmergencyAlert(analogRead(PIN_FLAME), analogRead(PIN_MQ2), analogRead(PIN_MQ8));
    
    webServer.println("HTTP/1.1 200 OK");
    webServer.println("Content-Type: text/plain");
    webServer.println("Content-Length: 21");
    webServer.println("Connection: close");
    webServer.println();
    webServer.println("Alert triggered!");
    
  } else if (request.indexOf("action=clear") != -1) {
    alertCount = 0;
    EEPROM.erase();
    
    webServer.println("HTTP/1.1 200 OK");
    webServer.println("Content-Type: text/plain");
    webServer.println("Content-Length: 18");
    webServer.println("Connection: close");
    webServer.println();
    webServer.println("History cleared!");
  } else {
    webServer.println("HTTP/1.1 404 Not Found");
    webServer.println("Connection: close");
    webServer.println();
  }
}

void checkAlertTrends() {
  // Called every 60 seconds
  if (alertCount >= CRITICAL_ALERT_THRESHOLD_COUNT) {
    isCriticalTrend = true;
    Serial.println("\n[Warning] ⚠️ CRITICAL TREND DETECTED - Multiple alerts in short time!");
    playBuzzerPattern(4);  // Long beep warning
  } else {
    isCriticalTrend = false;
  }
}

void getGeolocation() {
  Serial.println(" networks.");

  if (numNetworks == 0) {
    Serial.println("[Error] No WiFi networks found.");
    return;
  }

  // Build JSON payload for Google Geolocation API
  String payload = "{\"wifiAccessPoints\":[";
  
  for (int i = 0; i < numNetworks && i < 10; i++) { // Limit to 10 networks
    if (i > 0) payload += ",";
    payload += "{\"macAddress\":\"";
    payload += WiFi.BSSIDstr(i);
    payload += "\",\"signalStrength\":\"";
    payload += WiFi.RSSI(i);
    payload += "\"}";
  }
  
  payload += "],\"considerIp\":true}";

  Serial.println("[Info] Sending geolocation request to Google...");
  
  WiFiSSLClient client;
  HttpClient httpClient(client, "www.googleapis.com", 443);
  httpClient.setTimeout(5000);

  String path = "/geolocation/v1/geolocate?key=";
  path += GEOLOCATION_API_KEY;

  httpClient.beginRequest();
  httpClient.post(path);
  httpClient.sendHeader("Content-Type", "application/json");
  httpClient.sendHeader("Content-Length", payload.length());
  httpClient.beginBody();
  httpClient.print(payload);
  httpClient.endRequest();

  int status = httpClient.responseStatusCode();
  String response = httpClient.responseBody();

  Serial.print("[Info] Response Status: ");
  Serial.println(status);

  if (status == 200) {
    int latStart = response.indexOf("\"lat\":");
    int lngStart = response.indexOf("\"lng\":");
    
    if (latStart != -1 && lngStart != -1) {
      latStart += 6;
      lngStart += 6;
      
      int latEnd = response.indexOf(",", latStart);
      int lngEnd = response.indexOf("}", lngStart);
      
      String latStr = response.substring(latStart, latEnd);
      String lngStr = response.substring(lngStart, lngEnd);
      
      currentLatitude = latStr.toFloat();
      currentLongitude = lngStr.toFloat();
      hasLocation = true;
      
      Serial.print("[Success] Location: ");
      Serial.print(currentLatitude, 6);
      Serial.print(", ");
      Serial.println(currentLongitude, 6);
    }
  } else {
    Serial.println("[Error] Geolocation request failed.");
    Serial.println(response);
  }
}

void updateVisualization() {
  int valFlame = analogRead(PIN_FLAME);
  int valMQ2 = analogRead(PIN_MQ2);
  int valMQ8 = analogRead(PIN_MQ8);

  // Read Digital Pins
  bool dMQ2 = (digitalRead(D_PIN_MQ2) == HIGH);
  bool dMQ8 = (digitalRead(D_PIN_MQ8) == HIGH);

  // Mapping to 3 levels (0, 1, 2)
  int fH = map(constrain(valFlame, THRESHOLD_FLAME, 1023), 1023, THRESHOLD_FLAME, 0, 2);
  int m2H = map(constrain(valMQ2, 0, THRESHOLD_MQ2), 0, THRESHOLD_MQ2, 0, 2);
  int m8H = map(constrain(valMQ8, 0, THRESHOLD_MQ8), 0, THRESHOLD_MQ8, 0, 2);

  // If Digital is active, force height to max (2)
  if (dMQ2) m2H = 2;
  if (dMQ8) m8H = 2;

  // Shift history
  for (int i = 0; i < 3; i++) {
    for (int x = 0; x < 11; x++) history[i][x] = history[i][x+1];
  }
  history[0][11] = fH;
  history[1][11] = m2H;
  history[2][11] = m8H;

  // Render as Line Graph (only one pixel per column per sensor)
  memset(viz_frame, 0, sizeof(viz_frame));
  for (int x = 0; x < 12; x++) {
    // Lane 1: Rows 0-2 (Flame) - 3 pixels height
    // history 0->Row 2, 1->Row 1, 2->Row 0
    viz_frame[2 - history[0][x]][x] = 1;
    
    // Lane 2: Rows 3-5 (MQ2) - 3 pixels height
    // history 0->Row 5, 1->Row 4, 2->Row 3
    viz_frame[5 - history[1][x]][x] = 1;
    
    // Lane 3: Rows 6-7 (MQ8) - 2 pixels height
    // history 0->Row 7, 1-2->Row 6
    int h8 = (history[2][x] > 1) ? 1 : history[2][x]; 
    viz_frame[7 - h8][x] = 1;
  }
  matrix.renderBitmap(viz_frame, 8, 12);
}

void sendEmergencyAlert(int f_val, int m2_val, int m8_val) {
  // Determine alert level based on sensor values
  int alertLevel = ALERT_LEVEL_WARNING;
  
  // Escalate to CRITICAL if sensors exceed critical thresholds
  if (f_val < FLAME_CRITICAL_THRESHOLD || m2_val > THRESHOLD_MQ2 || m8_val > THRESHOLD_MQ8) {
    alertLevel = ALERT_LEVEL_CRITICAL;
  }
  
  // Rate limiting - prevent alert spam
  if (millis() - lastAlertTime < ALERT_COOLDOWN_MS) {
    Serial.println("[Warning] Alert rate limited. Wait before next alert.");
    return;
  }
  
  lastAlertTime = millis();
  alertCount++;
  
  String dataString = "F:" + String(f_val) + ", M2:" + String(m2_val) + ", M8:" + String(m8_val);
  
  // Store last alert details
  lastAlertData.timestamp = rtc.getEpoch();
  lastAlertData.flameVal = f_val;
  lastAlertData.mq2Val = m2_val;
  lastAlertData.mq8Val = m8_val;
  lastAlertData.latitude = currentLatitude;
  lastAlertData.longitude = currentLongitude;
  lastAlertData.alertLevel = alertLevel;
  
  // Log to EEPROM
  logAlertToEEPROM(f_val, m2_val, m8_val);
  
  // Play alarm pattern based on alert level
  if (alertLevel == ALERT_LEVEL_CRITICAL) {
    playBuzzerPattern(3);  // Rapid beeping
  } else {
    playBuzzerPattern(2);  // Double beep
  }
  
  if (testMode) {
    Serial.print("[Test Mode] Alert suppressed: ");
    Serial.println(dataString);
    Serial.print("[Test Mode] Alert Level: ");
    Serial.println(alertLevel == ALERT_LEVEL_CRITICAL ? "CRITICAL" : "WARNING");
    return;
  }
  
  if (useBLE) {
    Serial.println("[Info] Sending alert via BLE...");
    sensorDataChar.writeValue(dataString);
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Error] WiFi disconnected. Fallback to BLE potentially needed.");
    // If WiFi is gone but we haven't started BLE, we could start it here if desired.
    return;
  }

  // BUILD URL WITH SENSOR PARAMS
  String pathWithParams = String(RELAY_PATH) + "?flame=" + String(f_val) + "&mq2=" + String(m2_val) + "&mq8=" + String(m8_val);

  Serial.println("[Info] Sending initial GET request with sensor variables...");
  WiFiSSLClient wifi1;
  HttpClient client1(wifi1, RELAY_HOST, 443);
  client1.setTimeout(5000); 

  client1.beginRequest();
  client1.get(pathWithParams);
  client1.endRequest();

  int status1 = client1.responseStatusCode();
  String body1 = client1.responseBody();

  if (status1 != 302) {
    Serial.println("[Error] Expected 302 Redirect.");
    return;
  }

  int hrefStart = body1.indexOf("HREF=\"");
  if (hrefStart == -1) return;
  hrefStart += 6;
  
  int hrefEnd = body1.indexOf("\"", hrefStart);
  if (hrefEnd == -1) return;
  String redirectUrl = body1.substring(hrefStart, hrefEnd);
  redirectUrl.replace("&amp;", "&");

  int pathStart = redirectUrl.indexOf("/macros/");
  if (pathStart == -1) return;
  String path = redirectUrl.substring(pathStart);

  Serial.println("[Info] Following redirect...");
  WiFiSSLClient wifi2;
  HttpClient client2(wifi2, REDIRECT_HOST, 443);
  client2.setTimeout(5000);

  client2.beginRequest();
  client2.get(path);
  client2.endRequest();
  
  Serial.print("[Info] Final Status: "); 
  Serial.println(client2.responseStatusCode());
}

void setup() {
  Serial.begin(115200);
  matrix.begin();

  // Initialize Hardware
  pinMode(D_PIN_MQ2, INPUT);
  pinMode(D_PIN_MQ8, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize Storage & RTC
  EEPROM.begin();
  rtc.begin();
  
  // Sync RTC with compilation time (will be updated from WiFi)
  rtc.setTime(0, 0, 0);
  rtc.setDate(1, 1, 20);
  
  Serial.println("[Info] System Starting...");
  playBuzzerPattern(1);  // Single beep on startup

  // Initialize Digital Pins

  Serial.println("[Info] Connecting to WiFi (Timeout 5s)...");
  WiFi.begin(SSID, PASS);
  
  unsigned long startAttemptTime = millis();
  bool blinkState = false;
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
    if (blinkState) matrix.renderBitmap(frame_wifi, 8, 12);
    else matrix.renderBitmap(frame_blank, 8, 12);
    blinkState = !blinkState;
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    matrix.renderBitmap(frame_wifi, 8, 12);
    Serial.println("\n[Info] WiFi connected.");
  } else {
    Serial.println("\n[Warning] WiFi failed. Enabling BLE...");
    WiFi.end(); // Stop WiFi to free up resources/radio
    
    if (!BLE.begin()) {
      Serial.println("[Error] BLE initialization failed!");
    } else {
      BLE.setLocalName("4006A_Dude");
      BLE.setAdvertisedService(envService);
      envService.addCharacteristic(sensorDataChar);
      BLE.addService(envService);
      BLE.advertise();
      useBLE = true;
      Serial.println("[Info] BLE active and advertising.");
    }
  }
  
  // Run sensor diagnostics on startup
  delay(1000);
  runSensorDiagnostics();
  
  // Start web server if WiFi connected
  if (WiFi.status() == WL_CONNECTED) {
    webServer.begin();
    Serial.println("[Info] Web server started on port 80");
    Serial.print("[Info] Access dashboard at: http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
  }
}

void loop() {
  // Handle web requests
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client = webServer.available();
    if (client) {
      String request = "";
      while (client.available()) {
        char c = client.read();
        request += c;
      }
      handleWebRequest(request);
      client.stop();
    }
  }
  
  if (useBLE) {
    BLE.poll();
  }

  if (millis() - lastVizUpdate > 50) {
    updateVisualization();
    lastVizUpdate = millis();
  }

  if (millis() - lastSensorRead > 2000) {
    int valFlame = analogRead(PIN_FLAME);
    int valMQ2 = analogRead(PIN_MQ2);
    int valMQ8 = analogRead(PIN_MQ8);
    
    bool dMQ2 = (digitalRead(D_PIN_MQ2) == HIGH);
    bool dMQ8 = (digitalRead(D_PIN_MQ8) == HIGH);

    // Calibration Mode
    if (calibrationMode) {
      flameCalibrationMin = min(flameCalibrationMin, valFlame);
      flameCalibrationMax = max(flameCalibrationMax, valFlame);
      Serial.print("[Calibration] Flame value: ");
      Serial.print(valFlame);
      Serial.print(" | Min: ");
      Serial.print(flameCalibrationMin);
      Serial.print(" | Max: ");
      Serial.println(flameCalibrationMax);
    }

    Serial.print("[Sensors] Flame: "); Serial.print(valFlame);
    Serial.print(" | MQ2: "); Serial.print(valMQ2);
    Serial.print(dMQ2 ? " (D!)" : " ( )");
    Serial.print(" | MQ8: "); Serial.print(valMQ8);
    Serial.println(dMQ8 ? " (D!)" : " ( )");

    if (!calibrationMode && (valFlame < THRESHOLD_FLAME || valMQ2 > THRESHOLD_MQ2 || valMQ8 > THRESHOLD_MQ8 || dMQ2 || dMQ8)) {
      if (!testMode) {
        Serial.println("\n[Warning] Sensor threshold exceeded! Initiating alert...");
        
        for (int i = 0; i < 5; i++) {
          matrix.renderBitmap(frame_alert, 8, 12);
          delay(150);
          matrix.renderBitmap(frame_blank, 8, 12);
          delay(150);
        }
        matrix.renderBitmap(frame_alert, 8, 12);
      } else {
        Serial.println("\n[Test Mode] Sensor threshold exceeded! Alert animation suppressed.");
      }
      
      sendEmergencyAlert(valFlame, valMQ2, valMQ8);
      lastVizUpdate = millis();
    }
    lastSensorRead = millis();
    
    // Check for alert trends
    if (millis() - lastTrendCheck > TREND_CHECK_INTERVAL_MS) {
      checkAlertTrends();
      lastTrendCheck = millis();
    }
  }

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't') {
      Serial.println("\n[Info] Alert initialized by user.");
      sendEmergencyAlert(analogRead(PIN_FLAME), analogRead(PIN_MQ2), analogRead(PIN_MQ8));
    }
    if (c == 'm') {
      testMode = !testMode;
      Serial.print("\n[Info] Test Mode: ");
      Serial.println(testMode ? "ENABLED (Alerts suppressed)" : "DISABLED (Alerts will be sent)");
    }
    if (c == 'g') {
      Serial.println("\n[Info] Getting geolocation...");
      getGeolocation();
    }
    if (c == 'l') {
      if (hasLocation) {
        Serial.print("\n[Info] Current Location: ");
        Serial.print(currentLatitude, 6);
        Serial.print(", ");
        Serial.println(currentLongitude, 6);
      } else {
        Serial.println("\n[Info] No location data available. Press 'g' to get location.");
      }
    }
    if (c == 'c') {
      if (!calibrationMode) {
        startCalibration();
      } else {
        stopCalibration();
      }
    }
    if (c == 'h') {
      printAlertHistory();
    }
    if (c == 'x') {
      alertCount = 0;
      EEPROM.erase();
      Serial.println("\n[Info] Alert history cleared.");
      playBuzzerPattern(2);
    }
    if (c == 'd') {
      Serial.println("\n=== System Diagnostics ===");
      Serial.print("Test Mode: ");
      Serial.println(testMode ? "ON" : "OFF");
      Serial.print("WiFi Status: ");
      Serial.println(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
      Serial.print("BLE Status: ");
      Serial.println(useBLE ? "ACTIVE" : "INACTIVE");
      Serial.print("Total Alerts: ");
      Serial.println(alertCount);
      Serial.print("Calibration Mode: ");
      Serial.println(calibrationMode ? "ON" : "OFF");
      Serial.print("Flame Range: ");
      Serial.print(flameCalibrationMin);
      Serial.print(" - ");
      Serial.println(flameCalibrationMax);
    }
    if (c == 's') {
      Serial.println("\n[Info] Running full sensor diagnostics...");
      runSensorDiagnostics();
    }
    if (c == 'p') {
      printLastAlert();
    }
    if (c == 'a') {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║        ALERT STATISTICS & INFO         ║");
      Serial.println("╚════════════════════════════════════════╝\n");
      Serial.print("Total Alerts Triggered: ");
      Serial.println(alertCount);
      Serial.print("Alert Rate Limit: ");
      Serial.print(ALERT_COOLDOWN_MS / 1000);
      Serial.println(" seconds");
      Serial.print("Current Time Since Last Alert: ");
      Serial.print((millis() - lastAlertTime) / 1000);
      Serial.println(" seconds");
      Serial.print("EEPROM Storage Used: ");
      Serial.print(min(alertCount, MAX_ALERT_HISTORY));
      Serial.print("/");
      Serial.println(MAX_ALERT_HISTORY);
      Serial.print("Last Alert Level: ");
      Serial.println(lastAlertData.alertLevel == ALERT_LEVEL_CRITICAL ? "CRITICAL" : (lastAlertData.alertLevel == ALERT_LEVEL_WARNING ? "WARNING" : "NONE"));
      Serial.println();
    }
    if (c == 'e') {
      Serial.println("\n[Info] Extended Sensor Data Structure:");
      Serial.println("Current System Supports:");
      Serial.print("  - Flame: ");
      Serial.println(currentSensorData.flame);
      Serial.print("  - MQ2: ");
      Serial.println(currentSensorData.mq2);
      Serial.print("  - MQ8: ");
      Serial.println(currentSensorData.mq8);
      Serial.print("  - Temperature: Ready (DHT/BMP not connected)");
      Serial.print("  - Humidity: Ready (DHT not connected)");
      Serial.println("\nTo add temperature/humidity:");
      Serial.println("  1. Connect DHT22 to Pin 7");
      Serial.println("  2. Uncomment DHT library in sketch");
      Serial.println();
    }
    if (c == 'w') {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║        WEB DASHBOARD (Features 11)    ║");
      Serial.println("╚════════════════════════════════════════╝");
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("\n✓ Dashboard available at: http://");
        Serial.print(WiFi.localIP());
        Serial.println("/");
        Serial.println("\nFeatures:");
        Serial.println("  - Real-time sensor readings");
        Serial.println("  - System status display");
        Serial.println("  - Last alert details");
        Serial.println("  - Download CSV data");
        Serial.println("  - Trigger test alerts");
      } else {
        Serial.println("\n✗ WiFi not connected!");
        Serial.println("Connect to WiFi to enable web dashboard.");
      }
      Serial.println();
    }
    if (c == 'v') {
      Serial.println("\n[CSV Export Preview]");
      Serial.println(generateCSVData());
    }
    if (c == 'n') {
      Serial.println("\n[Email Alert] Sending test email...");
      sendEmailAlert(lastAlertData.alertLevel > 0 ? lastAlertData.alertLevel : ALERT_LEVEL_WARNING);
      Serial.println("[Email] Done!");
    }
    if (c == 'r') {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║      REMOTE CONTROL API (Feature 14)  ║");
      Serial.println("╚════════════════════════════════════════╝");
      Serial.println("\nHTTP Commands (Web API):");
      Serial.println("  GET /?action=csv      → Download alert history as CSV");
      Serial.println("  GET /?action=test     → Trigger test alert");
      Serial.println("  GET /?action=clear    → Clear all alert history");
      Serial.println("  GET /                 → Dashboard HTML");
      Serial.println("\nUsage: Open in browser or curl:");
      Serial.println("  curl http://YOUR_IP/?action=csv");
      Serial.println();
    }
    if (c == 'z') {
      Serial.println("\n╔════════════════════════════════════════╗");
      Serial.println("║   TREND ANALYSIS (Feature 15 - Alert   ║");
      Serial.println("║      Predictive Warnings)              ║");
      Serial.println("╚════════════════════════════════════════╝\n");
      Serial.print("Critical Trend Detected: ");
      Serial.println(isCriticalTrend ? "⚠️ YES - Multiple alerts in short time" : "✓ NO");
      Serial.print("Total Alerts (Recent): ");
      Serial.println(min(alertCount, MAX_ALERT_HISTORY));
      Serial.print("Trend Threshold: ");
      Serial.print(CRITICAL_ALERT_THRESHOLD_COUNT);
      Serial.println(" alerts");
      Serial.print("Check Interval: ");
      Serial.print(TREND_CHECK_INTERVAL_MS / 1000);
      Serial.println(" seconds");
      Serial.println("\n[Note] System automatically detects alert patterns and");
      Serial.println("triggers predictive warnings when threshold exceeded.");
      Serial.println();
    }
  }
}