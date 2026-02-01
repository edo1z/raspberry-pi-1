// ESP32 RC Car - Wi-Fi Remote Control
// ブラウザから操作できるラジコンカー

#include <WiFi.h>
#include <WebServer.h>

// Wi-Fi アクセスポイント設定
const char* ssid = "ESP32-RC-Car";
const char* password = "12345678";  // 8文字以上必要

// モーターピン設定
#define IN1 23
#define IN2 19
#define IN3 27
#define IN4 22

// Webサーバー（ポート80）
WebServer server(80);

void setup() {
  Serial.begin(115200);

  // モーターピン初期化
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotor();

  // Wi-Fi アクセスポイント開始
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println("=================================");
  Serial.println("ESP32 RC Car Started!");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  Serial.println(password);
  Serial.print("IP Address: ");
  Serial.println(IP);
  Serial.println("=================================");

  // エンドポイント設定
  server.on("/", handleRoot);
  server.on("/forward", handleForward);
  server.on("/backward", handleBackward);
  server.on("/left", handleLeft);
  server.on("/right", handleRight);
  server.on("/stop", handleStop);

  // CORS対応（クロスオリジンリクエスト許可）
  server.enableCORS(true);

  // サーバー開始
  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  server.handleClient();
}

// ルートページ
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>ESP32 RC Car</title></head><body>";
  html += "<h1>ESP32 RC Car</h1>";
  html += "<p>Use endpoints: /forward, /backward, /left, /right, /stop</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// 前進
void handleForward() {
  Serial.println("Forward");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  server.send(200, "text/plain", "OK: Forward");
}

// 後退
void handleBackward() {
  Serial.println("Backward");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  server.send(200, "text/plain", "OK: Backward");
}

// 左旋回
void handleLeft() {
  Serial.println("Left");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  server.send(200, "text/plain", "OK: Left");
}

// 右旋回
void handleRight() {
  Serial.println("Right");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  server.send(200, "text/plain", "OK: Right");
}

// 停止
void handleStop() {
  Serial.println("Stop");
  stopMotor();
  server.send(200, "text/plain", "OK: Stop");
}

// モーター停止
void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
