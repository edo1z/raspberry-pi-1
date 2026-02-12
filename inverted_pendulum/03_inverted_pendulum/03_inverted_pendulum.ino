/*
 * 倒立振子ロボット
 * ESP32 + DRV8833 + MPU6050
 *
 * ESP32 Arduino Core 3.x対応版
 * ライブラリ不要版（直接I2C通信）
 * Wi-Fiアクセスポイント経由でPIDチューニング＆ラジコン操作
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
 *   2. Safariで http://192.168.4.1 → PIDチューニング
 *   3. Safariで http://192.168.4.1/rc → ラジコン操作
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
float Kp = 42.0;
float Ki = 2.1;
float Kd = 2.5;

// ========== 目標角度 ==========
float targetAngle = 0.0;    // PID画面で設定するバランス点
float baseTargetAngle = 0.0; // バランス点の基準値

// ========== ラジコン操作 ==========
float rcForward = 0.0;   // 前後 -1.0 ~ 1.0 (前進が正)
float rcTurn = 0.0;      // 旋回 -1.0 ~ 1.0 (右が正)
float RC_ANGLE_MAX = 3.0;   // 前後操作で傾ける最大角度
float RC_TURN_SPEED = 80.0; // 旋回時の左右モーター速度差

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

// ========== PIDチューニングページ ==========
const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>BalanceBot PID</title>
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
.nav{text-align:center;margin-bottom:12px}
.nav a{color:#0ff;font-size:16px}
</style>
</head>
<body>
<div class="nav"><a href="/rc">RC Mode &gt;&gt;</a></div>
<h2>PID Tuning</h2>
<div class="p">
  <label>Kp: <span id="vp" class="v">42.00</span></label>
  <input type="range" id="sp" min="0" max="100" step="0.5" value="42"
    oninput="document.getElementById('vp').textContent=this.value;send('P'+this.value)">
</div>
<div class="p">
  <label>Ki: <span id="vi" class="v">2.10</span></label>
  <input type="range" id="si" min="0" max="10" step="0.1" value="2.1"
    oninput="document.getElementById('vi').textContent=this.value;send('I'+this.value)">
</div>
<div class="p">
  <label>Kd: <span id="vd" class="v">2.50</span></label>
  <input type="range" id="sd" min="0" max="20" step="0.1" value="2.5"
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

// ========== ラジコン操作ページ ==========
const char RC_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>BalanceBot RC</title>
<style>
*{box-sizing:border-box;-webkit-user-select:none;user-select:none}
body{font-family:sans-serif;margin:0;padding:16px;background:#1a1a2e;color:#eee;
  overflow:hidden;touch-action:none}
h2{text-align:center;color:#0ff;margin:8px 0}
.nav{text-align:center;margin-bottom:8px}
.nav a{color:#0ff;font-size:16px}
#status{text-align:center;font-size:14px;color:#0f0;margin:4px 0}
#pad{position:relative;width:280px;height:280px;margin:16px auto;
  background:radial-gradient(circle,#223 0%,#112 100%);
  border:2px solid #0ff;border-radius:50%}
#stick{position:absolute;width:70px;height:70px;background:radial-gradient(circle,#0ff,#066);
  border-radius:50%;left:105px;top:105px;pointer-events:none;
  box-shadow:0 0 15px #0ff}
#arrows{position:absolute;width:100%;height:100%;top:0;left:0;pointer-events:none}
#arrows span{position:absolute;font-size:24px;color:rgba(0,255,255,0.3)}
#au{top:12px;left:50%;transform:translateX(-50%)}
#ad{bottom:12px;left:50%;transform:translateX(-50%)}
#al{left:12px;top:50%;transform:translateY(-50%)}
#ar{right:12px;top:50%;transform:translateY(-50%)}
#info{text-align:center;font-size:13px;margin-top:8px;color:#888}
</style>
</head>
<body>
<div class="nav"><a href="/">&lt;&lt; PID Tuning</a></div>
<h2>RC Mode</h2>
<div id="status">Angle: -- | Output: --</div>
<div id="pad">
  <div id="arrows">
    <span id="au">&uarr;</span>
    <span id="ad">&darr;</span>
    <span id="al">&larr;</span>
    <span id="ar">&rarr;</span>
  </div>
  <div id="stick"></div>
</div>
<div id="info">Drag to move</div>
<script>
var pad=document.getElementById('pad');
var stick=document.getElementById('stick');
var cx=140,cy=140,maxR=105;
var touching=false,tid=null;
var curFwd=0,curTurn=0;

function pos(px,py){
  var dx=px-cx,dy=py-cy;
  var dist=Math.sqrt(dx*dx+dy*dy);
  if(dist>maxR){dx=dx/dist*maxR;dy=dy/dist*maxR;}
  stick.style.left=(cx+dx-35)+'px';
  stick.style.top=(cy+dy-35)+'px';
  curFwd=(-dy/maxR);
  curTurn=(dx/maxR);
  curFwd=Math.round(curFwd*100)/100;
  curTurn=Math.round(curTurn*100)/100;
}

function reset(){
  stick.style.left='105px';stick.style.top='105px';
  curFwd=0;curTurn=0;
  sendRC();
}

function sendRC(){fetch('/steer?f='+curFwd+'&t='+curTurn);}

function getXY(e){
  var r=pad.getBoundingClientRect();
  var t=e.touches?e.touches[0]:e;
  return{x:t.clientX-r.left,y:t.clientY-r.top};
}

pad.addEventListener('touchstart',function(e){
  e.preventDefault();touching=true;
  var p=getXY(e);pos(p.x,p.y);sendRC();
  if(!tid)tid=setInterval(sendRC,100);
});
pad.addEventListener('touchmove',function(e){
  e.preventDefault();if(!touching)return;
  var p=getXY(e);pos(p.x,p.y);
});
pad.addEventListener('touchend',function(e){
  e.preventDefault();touching=false;reset();
  if(tid){clearInterval(tid);tid=null;}
});
pad.addEventListener('mousedown',function(e){
  touching=true;var p=getXY(e);pos(p.x,p.y);sendRC();
  if(!tid)tid=setInterval(sendRC,100);
});
document.addEventListener('mousemove',function(e){
  if(!touching)return;var p=getXY(e);pos(p.x,p.y);
});
document.addEventListener('mouseup',function(){
  if(!touching)return;touching=false;reset();
  if(tid){clearInterval(tid);tid=null;}
});

var es=new EventSource('/events');
es.onmessage=function(e){document.getElementById('status').textContent=e.data;};
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
    case 'T': case 't': baseTargetAngle = val; break;
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
  Serial.print("  target="); Serial.println(baseTargetAngle, 2);
}

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

void handleRC() {
  server.send(200, "text/html", RC_PAGE);
}

void handleCmd() {
  if (server.hasArg("c")) {
    processCommand(server.arg("c"));
  }
  server.send(200, "text/plain", "OK");
}

void handleSteer() {
  if (server.hasArg("f")) rcForward = constrain(server.arg("f").toFloat(), -1.0, 1.0);
  if (server.hasArg("t")) rcTurn = constrain(server.arg("t").toFloat(), -1.0, 1.0);
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
  sseClient.clear();
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
  server.on("/rc", handleRC);
  server.on("/cmd", handleCmd);
  server.on("/steer", handleSteer);
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
  Serial.println("  PID:  http://192.168.4.1");
  Serial.println("  RC:   http://192.168.4.1/rc");
}

// デバッグ出力の間引き用
unsigned long lastSSETime = 0;
const int SSE_INTERVAL_MS = 100;

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

  // ========== ラジコン操作を目標角度に反映 ==========
  targetAngle = baseTargetAngle + rcForward * RC_ANGLE_MAX;

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

  // ========== 旋回を加える ==========
  int turnOffset = (int)(rcTurn * RC_TURN_SPEED);
  int motorA = constrain(-motorPWM - turnOffset, -255, 255);
  int motorB = constrain( motorPWM - turnOffset, -255, 255);

  // ========== モーター駆動 ==========
  setMotors(motorA, motorB);

  // ========== デバッグ出力 ==========
  String dbg = "Angle:" + String(angle, 1) + " Err:" + String(error, 1) + " Out:" + String(motorPWM);
  Serial.println(dbg);

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
