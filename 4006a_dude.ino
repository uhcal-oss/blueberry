#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
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
const int PIN_MQ7 = A2;

// Thresholds for sensors
const int THRESHOLD_FLAME = 500;
const int THRESHOLD_MQ2 = 400;
const int THRESHOLD_MQ7 = 400;

// Pong Game Variables
uint8_t pong_frame[8][12];
int ball_x = 5, ball_y = 3, ball_dx = 1, ball_dy = 1;
int paddle1_y = 2, paddle2_y = 2;

unsigned long lastPongUpdate = 0;
unsigned long lastSensorRead = 0;

void updatePong() {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 12; x++) pong_frame[y][x] = 0;
  }

  ball_x += ball_dx; ball_y += ball_dy;
  if (ball_y <= 0 || ball_y >= 7) ball_dy = -ball_dy;

  if (ball_x < 6) {
    if (paddle1_y + 1 < ball_y && paddle1_y < 5) paddle1_y++;
    else if (paddle1_y + 1 > ball_y && paddle1_y > 0) paddle1_y--;
  }
  if (ball_x > 5) {
    if (paddle2_y + 1 < ball_y && paddle2_y < 5) paddle2_y++;
    else if (paddle2_y + 1 > ball_y && paddle2_y > 0) paddle2_y--;
  }

  if (ball_x == 1 && ball_y >= paddle1_y && ball_y <= paddle1_y + 2) ball_dx = -ball_dx;
  else if (ball_x == 10 && ball_y >= paddle2_y && ball_y <= paddle2_y + 2) ball_dx = -ball_dx;

  if (ball_x < 0) { ball_x = 10; ball_y = paddle2_y + 1; }
  if (ball_x > 11) { ball_x = 1; ball_y = paddle1_y + 1; }

  for (int i = 0; i < 3; i++) {
    if (paddle1_y + i >= 0 && paddle1_y + i < 8) pong_frame[paddle1_y + i][0] = 1;
    if (paddle2_y + i >= 0 && paddle2_y + i < 8) pong_frame[paddle2_y + i][11] = 1;
  }

  if (ball_y >= 0 && ball_y < 8 && ball_x >= 0 && ball_x < 12) pong_frame[ball_y][ball_x] = 1;
  matrix.renderBitmap(pong_frame, 8, 12);
}

const char* SSID = "FiberHGW_ZYE96B";
const char* PASS = "nw9arYtmRzHv";

const char* RELAY_HOST = "script.google.com";
const char* REDIRECT_HOST = "script.googleusercontent.com";
const char* RELAY_PATH = "/macros/s/AKfycbxRwfIpFvgHKkhzipOpcHrb9ju7xYoqvCE76X7QbDC-wPKjXweml59ru7hTRU6CWRlS/exec";

void sendEmergencyAlert(int f_val, int m2_val, int m7_val) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[Error] WiFi disconnected. Attempting to reconnect...");
    WiFi.begin(SSID, PASS);
    return;
  }

  // BUILD URL WITH SENSOR PARAMS
  String pathWithParams = String(RELAY_PATH) + "?flame=" + String(f_val) + "&mq2=" + String(m2_val) + "&mq7=" + String(m7_val);

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

  Serial.println("[Info] Connecting to WiFi...");
  WiFi.begin(SSID, PASS);
  
  bool blinkState = false;
  while (WiFi.status() != WL_CONNECTED) {
    if (blinkState) matrix.renderBitmap(frame_wifi, 8, 12);
    else matrix.renderBitmap(frame_blank, 8, 12);
    blinkState = !blinkState;
    delay(500);
    Serial.print(".");
  }

  matrix.renderBitmap(frame_wifi, 8, 12);
  Serial.println("\n[Info] WiFi connected.");
}

void loop() {
  if (millis() - lastPongUpdate > 150) {
    updatePong();
    lastPongUpdate = millis();
  }

  if (millis() - lastSensorRead > 2000) {
    int valFlame = analogRead(PIN_FLAME);
    int valMQ2 = analogRead(PIN_MQ2);
    int valMQ7 = analogRead(PIN_MQ7);

    Serial.print("[Sensors] Flame: "); Serial.print(valFlame);
    Serial.print(" | MQ2: "); Serial.print(valMQ2);
    Serial.print(" | MQ7: "); Serial.println(valMQ7);

    if (valFlame < THRESHOLD_FLAME || valMQ2 > THRESHOLD_MQ2 || valMQ7 > THRESHOLD_MQ7) {
      Serial.println("\n[Warning] Sensor threshold exceeded! Initiating alert...");
      
      for (int i = 0; i < 5; i++) {
        matrix.renderBitmap(frame_alert, 8, 12);
        delay(150);
        matrix.renderBitmap(frame_blank, 8, 12);
        delay(150);
      }
      matrix.renderBitmap(frame_alert, 8, 12);
      
      sendEmergencyAlert(valFlame, valMQ2, valMQ7);
      lastPongUpdate = millis();
    }
    lastSensorRead = millis();
  }

  if (Serial.available()) {
    char c = Serial.read();
    if (c == 't') {
      Serial.println("\n[Info] Alert initialized by user.");
      sendEmergencyAlert(analogRead(PIN_FLAME), analogRead(PIN_MQ2), analogRead(PIN_MQ7));
    }
  }
}