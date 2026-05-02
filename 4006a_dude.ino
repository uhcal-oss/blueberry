#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoBLE.h>
#include "Arduino_LED_Matrix.h"

ArduinoLEDMatrix matrix;

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

const int D_PIN_FLAME = 2; // Digital Flame Pin
const int D_PIN_MQ2 = 3;   // Digital MQ2 Pin
const int D_PIN_MQ8 = 4;   // Digital MQ8 Pin

// Thresholds for sensors
const int THRESHOLD_FLAME = 100;
const int THRESHOLD_MQ2 = 700;
const int THRESHOLD_MQ8 = 400;

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

void updateVisualization() {
  int valFlame = analogRead(PIN_FLAME);
  int valMQ2 = analogRead(PIN_MQ2);
  int valMQ8 = analogRead(PIN_MQ8);

  // Read Digital Pins
  bool dFlame = (digitalRead(D_PIN_FLAME) == LOW); // Flame sensors are usually active LOW
  bool dMQ2 = (digitalRead(D_PIN_MQ2) == HIGH);
  bool dMQ8 = (digitalRead(D_PIN_MQ8) == HIGH);

  // Mapping to 3 levels (0, 1, 2)
  int fH = map(constrain(valFlame, THRESHOLD_FLAME, 1023), 1023, THRESHOLD_FLAME, 0, 2);
  int m2H = map(constrain(valMQ2, 0, THRESHOLD_MQ2), 0, THRESHOLD_MQ2, 0, 2);
  int m8H = map(constrain(valMQ8, 0, THRESHOLD_MQ8), 0, THRESHOLD_MQ8, 0, 2);

  // If Digital is active, force height to max (2)
  if (dFlame) fH = 2;
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

const char* SSID = "FiberHGW_ZYE96B";
const char* PASS = "nw9arYtmRzHv";

const char* RELAY_HOST = "script.google.com";
const char* REDIRECT_HOST = "script.googleusercontent.com";
const char* RELAY_PATH = "/macros/s/AKfycbxRwfIpFvgHKkhzipOpcHrb9ju7xYoqvCE76X7QbDC-wPKjXweml59ru7hTRU6CWRlS/exec";

void sendEmergencyAlert(int f_val, int m2_val, int m8_val) {
  String dataString = "F:" + String(f_val) + ", M2:" + String(m2_val) + ", M8:" + String(m8_val);
  
  if (testMode) {
    Serial.print("[Test Mode] Alert suppressed: ");
    Serial.println(dataString);
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

  // Initialize Digital Pins
  pinMode(D_PIN_FLAME, INPUT_PULLUP);
  pinMode(D_PIN_MQ2, INPUT);
  pinMode(D_PIN_MQ8, INPUT);

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
}

void loop() {
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
    
    bool dFlame = (digitalRead(D_PIN_FLAME) == LOW);
    bool dMQ2 = (digitalRead(D_PIN_MQ2) == HIGH);
    bool dMQ8 = (digitalRead(D_PIN_MQ8) == HIGH);

    Serial.print("[Sensors] Flame: "); Serial.print(valFlame);
    Serial.print(dFlame ? " (D!)" : " ( )");
    Serial.print(" | MQ2: "); Serial.print(valMQ2);
    Serial.print(dMQ2 ? " (D!)" : " ( )");
    Serial.print(" | MQ8: "); Serial.print(valMQ8);
    Serial.println(dMQ8 ? " (D!)" : " ( )");

    if (valFlame < THRESHOLD_FLAME || valMQ2 > THRESHOLD_MQ2 || valMQ8 > THRESHOLD_MQ8 || dFlame || dMQ2 || dMQ8) {
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
  }
}