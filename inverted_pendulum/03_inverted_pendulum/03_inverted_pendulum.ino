/*
 * 倒立振子ロボット
 * ESP32 + DRV8833 + MPU6050
 *
 * ESP32 Arduino Core 3.x対応版
 * ライブラリ不要版（直接I2C通信）
 * Wi-Fiアクセスポイント経由でPIDチューニング
 *
 * 接続:
 *   モーター (DRV8833):
 *     GPIO16 → AIN1
 *     GPIO17 → AIN2
 *     GPIO18 → BIN1
 *     GPIO19 → BIN2
 *     3.3V   → STBY
 *
 *   傾きセンサー (MPU6050):
 *     GPIO21 → SDA
 *     GPIO22 → SCL
 *     3.3V   → VCC
 *     GND    → GND
 *
 * 使い方:
 *   1. iPhoneのWi-Fi設定で「BalanceBot」に接続（パスワード: 12345678）
 *   2. Safariで http://192.168.4.1 を開く
 */

#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

// ========== Wi-Fi AP設定 ==========
const char* AP_SSID = "BalanceBot";
const char* AP_PASS = "12345678";

WebServer server(80);

// ========== MPU6050設定 ==========
#define MPU6050_ADDR 0x68
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B
#define WHO_AM_I     0x75

// ========== ピン定義 ==========
#define AIN1 16
#define AIN2 17
#define BIN1 18
#define BIN2 19

// ========== PWM設定 ==========
#define PWM_FREQ 1000
#define PWM_RESOLUTION 8

// ========== PIDパラメータ ==========
float Kp = 30.0;
float Ki = 0.5;
float Kd = 1.5;

// ========== 目標角度 ==========
float targetAngle = 0.0;

// ========== センサー・制御変数 ==========
float angle = 0;
float prevAngle = 0;
float integral = 0;
unsigned long prevTime = 0;

const float ALPHA = 0.98;
const int CONTROL_PERIOD_MS = 10;
const float SAFETY_ANGLE = 45.0;

// シリアルコマンド用バッファ
String inputBuffer = "";

// ========== Webページ ==========
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>BalanceBot</title>
<style>
body{font-family:sans-serif;margin:16px;background:#1a1a2e;color:#eee}
h2{text-align:center;color:#0ff}
.p{margin:12px 0}
label{display:block;font-size:14px;margin-bottom:2px}
input[type=range]{width:100%}
.v{font-weight:bold;color:#0ff;font-size:18px}
#log{background:#000;color:#0f0;font-family:monospace;font-size:12px;
  height:150px;overflow-y:auto;padding:8px;border-radius:4px;margin-top:12px;
  white-space:pre}
button{background:#0ff;color:#000;border:none;padding:8px 16px;
  border-radius:4px;font-size:14px;margin:4px;cursor:pointer}
button:active{background:#099}
</style>
</head>
<body>
<h2>BalanceBot PID</h2>
<div class="p">
  <label>Kp: <span id="vp" class="v">30.00</span></label>
  <input type="range" id="sp" min="0" max="100" step="0.5" value="30"
    oninput="document.getElementById('vp').textContent=this.value;send('P'+this.value)">
</div>
<div class="p">
  <label>Ki: <span id="vi" class="v">0.50</span></label>
  <input type="range" id="si" min="0" max="10" step="0.1" value="0.5"
    oninput="document.getElementById('vi').textContent=this.value;send('I'+this.value)">
</div>
<div class="p">
  <label>Kd: <span id="vd" class="v">1.50</span></label>
  <input type="range" id="sd" min="0" max="20" step="0.1" value="1.5"
    oninput="document.getElementById('vd').textContent=this.value;send('D'+this.value)">
</div>
<div class="p">
  <label>Target Angle: <span id="vt" class="v">0.00</span></label>
  <input type="range" id="st" min="-10" max="10" step="0.1" value="0"
    oninput="document.getElementById('vt').textContent=this.value;send('T'+this.value)">
</div>
<button onclick="send('S')">Show</button>
<div id="log"></div>
<script>
var es;
function init(){
  es=new EventSource('/events');
  es.onmessage=function(e){
    var d=document.getElementById('log');
    d.textContent+=e.data+'\n';
    if(d.childNodes.length>200)d.textContent=d.textContent.split('\n').slice(-100).join('\n');
    d.scrollTop=d.scrollHeight;
  };
}
function send(c){fetch('/cmd?c='+encodeURIComponent(c));}
init();
</script>
</body>
</html>
)rawliteral";

// SSE クライアント
WiFiClient sseClient;
bool sseConnected = false;

void processCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  char type = cmd.charAt(0);
  float val = cmd.substring(1).toFloat();

  switch (type) {
    case 'P': case 'p': Kp = val; break;
    case 'I': case 'i': Ki = val; break;
    case 'D': case 'd': Kd = val; break;
    case 'T': case 't': targetAngle = val; break;
    case 'S': case 's': break;
    default:
      Serial.println("Unknown command");
      return;
  }
  integral = 0;
  Serial.println("--- Current PID ---");
  Serial.print("  Kp="); Serial.println(Kp, 2);
  Serial.print("  Ki="); Serial.println(Ki, 2);
  Serial.print("  Kd="); Serial.println(Kd, 2);
  Serial.print("  target="); Serial.println(targetAngle, 2);
}

// SSEでデータ送信
void sendSSE(const String &data) {
  if (sseConnected && sseClient.connected()) {
    sseClient.print("data: ");
    sseClient.println(data);
    sseClient.println();
  } else {
    sseConnected = false;
  }
}

// Webサーバーハンドラ
void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleCmd() {
  if (server.hasArg("c")) {
    processCommand(server.arg("c"));
  }
  server.send(200, "text/plain", "OK");
}

void handleEvents() {
  sseClient = server.client();
  sseClient.println("HTTP/1.1 200 OK");
  sseClient.println("Content-Type: text/event-stream");
  sseClient.println("Cache-Control: no-cache");
  sseClient.println("Connection: keep-alive");
  sseClient.println("Access-Control-Allow-Origin: *");
  sseClient.println();
  sseClient.flush();
  sseConnected = true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Inverted Pendulum Start!");

  // Wi-Fi AP起動
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("Wi-Fi AP: ");
  Serial.println(AP_SSID);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Webサーバー設定
  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);
  server.on("/events", handleEvents);
  server.begin();

  // I2C初期化（PWMより先に行う）
  Wire.begin(21, 22);

  // MPU6050の存在確認
  Serial.println("Initializing MPU6050...");
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(WHO_AM_I);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 1);

  if (Wire.available()) {
    byte whoami = Wire.read();
    Serial.print("WHO_AM_I: 0x");
    Serial.println(whoami, HEX);

    if (whoami == 0x68 || whoami == 0x70 || whoami == 0x98 || whoami == 0x71) {
      Serial.println("MPU6050 OK!");
    } else {
      Serial.println("Unknown device, but trying anyway...");
    }
  } else {
    Serial.println("MPU6050 not responding!");
    while (1);
  }

  // MPU6050をスリープ解除
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(PWR_MGMT_1);
  Wire.write(0x00);
  Wire.endTransmission();

  // PWM設定
  ledcAttach(AIN1, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(AIN2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(BIN1, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(BIN2, PWM_FREQ, PWM_RESOLUTION);

  stopMotors();

  Serial.println("Place robot upright and wait...");
  delay(3000);

  prevTime = millis();
  Serial.println("GO!");
  Serial.println("Connect to Wi-Fi 'BalanceBot' (pass: 12345678)");
  Serial.println("Then open http://192.168.4.1");
}

// デバッグ出力の間引き用
unsigned long lastSSETime = 0;
const int SSE_INTERVAL_MS = 100;  // 10Hz でブラウザに送信

void loop() {
  // Webサーバー処理
  server.handleClient();

  // シリアルコマンド受信（USB）
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      processCommand(inputBuffer);
      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }

  unsigned long currentTime = millis();

  // 制御周期チェック
  if (currentTime - prevTime < CONTROL_PERIOD_MS) {
    return;
  }

  float dt = (currentTime - prevTime) / 1000.0;
  prevTime = currentTime;

  // ========== 角度取得 ==========
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 14);

  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();
  int16_t temp = (Wire.read() << 8) | Wire.read();
  int16_t gx = (Wire.read() << 8) | Wire.read();
  int16_t gy = (Wire.read() << 8) | Wire.read();
  int16_t gz = (Wire.read() << 8) | Wire.read();

  float accelAngle = atan2(ay, az) * 180.0 / PI;
  float gyroRate = gx / 131.0;
  angle = ALPHA * (angle + gyroRate * dt) + (1.0 - ALPHA) * accelAngle;

  // ========== 安全チェック ==========
  if (abs(angle - targetAngle) > SAFETY_ANGLE) {
    stopMotors();
    Serial.println("!! SAFETY STOP !!");
    sendSSE("!! SAFETY STOP !!");
    delay(500);
    return;
  }

  // ========== PID制御 ==========
  float error = angle - targetAngle;

  integral += error * dt;
  integral = constrain(integral, -100, 100);

  float derivative = (angle - prevAngle) / dt;
  prevAngle = angle;

  float output = Kp * error + Ki * integral + Kd * derivative;
  int motorPWM = constrain((int)output, -255, 255);

  // ========== モーター駆動 ==========
  setMotors(-motorPWM, motorPWM);

  // ========== デバッグ出力 ==========
  String dbg = "Angle:" + String(angle, 1) + " Err:" + String(error, 1) + " Out:" + String(motorPWM);
  Serial.println(dbg);

  // SSEは間引いて送信（制御ループを遅くしない）
  if (currentTime - lastSSETime >= SSE_INTERVAL_MS) {
    sendSSE(dbg);
    lastSSETime = currentTime;
  }
}

void setMotors(int speedA, int speedB) {
  if (speedA >= 0) {
    ledcWrite(AIN1, speedA);
    ledcWrite(AIN2, 0);
  } else {
    ledcWrite(AIN1, 0);
    ledcWrite(AIN2, -speedA);
  }

  if (speedB >= 0) {
    ledcWrite(BIN1, speedB);
    ledcWrite(BIN2, 0);
  } else {
    ledcWrite(BIN1, 0);
    ledcWrite(BIN2, -speedB);
  }
}

void stopMotors() {
  ledcWrite(AIN1, 0);
  ledcWrite(AIN2, 0);
  ledcWrite(BIN1, 0);
  ledcWrite(BIN2, 0);
}
