/*
 * MPU6050 傾き取得テスト
 * ESP32 + MPU6050
 *
 * 接続:
 *   GPIO21 → SDA
 *   GPIO22 → SCL
 *   3.3V   → VCC
 *   GND    → GND
 *
 * ライブラリ:
 *   Arduino IDEで「MPU6050」をインストール
 *   (by Electronic Cats)
 */

#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// 相補フィルタ用
float angle = 0;
unsigned long prevTime = 0;
const float ALPHA = 0.98;  // 相補フィルタ係数

void setup() {
  Serial.begin(115200);
  Serial.println("MPU6050 Test Start!");

  // I2C初期化
  Wire.begin(21, 22);  // SDA=21, SCL=22

  // MPU6050初期化
  Serial.println("Initializing MPU6050...");
  mpu.initialize();

  // 接続確認
  if (mpu.testConnection()) {
    Serial.println("MPU6050 connection successful!");
  } else {
    Serial.println("MPU6050 connection failed!");
    while (1);
  }

  // キャリブレーション（静止状態で実行）
  Serial.println("Calibrating... Keep the sensor still!");
  delay(1000);

  prevTime = millis();
}

void loop() {
  // 生データ取得
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // 時間計算
  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0;
  prevTime = currentTime;

  // 加速度から角度計算（度）
  float accelAngle = atan2(ay, az) * 180.0 / PI;

  // ジャイロから角速度取得（度/秒）
  // MPU6050のデフォルト感度: 131 LSB/(°/s)
  float gyroRate = gx / 131.0;

  // 相補フィルタで角度算出
  angle = ALPHA * (angle + gyroRate * dt) + (1.0 - ALPHA) * accelAngle;

  // 結果表示
  Serial.print("Angle: ");
  Serial.print(angle, 1);
  Serial.print(" deg  |  AccelAngle: ");
  Serial.print(accelAngle, 1);
  Serial.print(" deg  |  GyroRate: ");
  Serial.print(gyroRate, 1);
  Serial.println(" deg/s");

  delay(20);  // 50Hz
}
