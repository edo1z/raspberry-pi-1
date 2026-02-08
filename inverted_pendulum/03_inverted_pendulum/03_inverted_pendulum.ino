/*
 * 倒立振子ロボット
 * ESP32 + DRV8833 + MPU6050
 *
 * ESP32 Arduino Core 3.x対応版
 * ライブラリ不要版（直接I2C通信）
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
 */

#include <Wire.h>

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
// ※要調整！ロボットの重心・モーターに合わせてチューニング
float Kp = 30.0;   // 比例ゲイン
float Ki = 0.5;    // 積分ゲイン
float Kd = 1.5;    // 微分ゲイン

// ========== 目標角度 ==========
float targetAngle = 0.0;  // バランス点（度）※要調整

// ========== センサー・制御変数 ==========
float angle = 0;
float prevAngle = 0;
float integral = 0;
unsigned long prevTime = 0;

// 相補フィルタ係数
const float ALPHA = 0.98;

// 制御周期
const int CONTROL_PERIOD_MS = 10;  // 100Hz

// 安全停止角度
const float SAFETY_ANGLE = 45.0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Inverted Pendulum Start!");

  // ESP32 Arduino Core 3.x: ledcAttach(pin, freq, resolution)
  ledcAttach(AIN1, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(AIN2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(BIN1, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(BIN2, PWM_FREQ, PWM_RESOLUTION);

  stopMotors();

  // I2C初期化
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

    if (whoami == 0x68 || whoami == 0x98) {
      Serial.println("MPU6050 OK!");
    } else {
      Serial.println("Unknown device!");
      while (1);
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

  // キャリブレーション待ち
  Serial.println("Place robot upright and wait...");
  delay(3000);

  prevTime = millis();
  Serial.println("GO!");
}

void loop() {
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
  int16_t temp = (Wire.read() << 8) | Wire.read();  // 温度（未使用）
  int16_t gx = (Wire.read() << 8) | Wire.read();
  int16_t gy = (Wire.read() << 8) | Wire.read();
  int16_t gz = (Wire.read() << 8) | Wire.read();

  // 加速度から角度
  float accelAngle = atan2(ay, az) * 180.0 / PI;

  // ジャイロから角速度
  float gyroRate = gx / 131.0;

  // 相補フィルタ
  angle = ALPHA * (angle + gyroRate * dt) + (1.0 - ALPHA) * accelAngle;

  // ========== 安全チェック ==========
  if (abs(angle - targetAngle) > SAFETY_ANGLE) {
    stopMotors();
    Serial.println("!! SAFETY STOP !!");
    delay(500);
    return;
  }

  // ========== PID制御 ==========
  float error = angle - targetAngle;

  // 積分（リセット機能付き）
  integral += error * dt;
  integral = constrain(integral, -100, 100);  // 積分上限

  // 微分
  float derivative = (angle - prevAngle) / dt;
  prevAngle = angle;

  // 出力計算
  float output = Kp * error + Ki * integral + Kd * derivative;

  // PWM値に変換
  int motorPWM = constrain((int)output, -255, 255);

  // ========== モーター駆動 ==========
  setMotors(motorPWM, motorPWM);

  // ========== デバッグ出力 ==========
  Serial.print("Angle:");
  Serial.print(angle, 1);
  Serial.print(" Err:");
  Serial.print(error, 1);
  Serial.print(" Out:");
  Serial.print(motorPWM);
  Serial.println();
}

/**
 * 両モーターの速度を設定
 * ESP32 Core 3.x: ledcWrite(pin, duty)
 */
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

/**
 * 両モーター停止
 */
void stopMotors() {
  ledcWrite(AIN1, 0);
  ledcWrite(AIN2, 0);
  ledcWrite(BIN1, 0);
  ledcWrite(BIN2, 0);
}
