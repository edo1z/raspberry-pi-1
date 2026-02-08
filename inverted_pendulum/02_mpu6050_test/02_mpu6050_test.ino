/*
 * MPU6050 傾き取得テスト
 * ESP32 + MPU6050
 *
 * ライブラリ不要版（直接I2C通信）
 *
 * 接続:
 *   GPIO21 → SDA
 *   GPIO22 → SCL
 *   3.3V   → VCC
 *   GND    → GND
 */

#include <Wire.h>

// MPU6050 I2Cアドレス
#define MPU6050_ADDR 0x68

// レジスタアドレス
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B
#define WHO_AM_I     0x75

// 相補フィルタ用
float angle = 0;
unsigned long prevTime = 0;
const float ALPHA = 0.98;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("MPU6050 Test Start!");

  // I2C初期化
  Wire.begin(21, 22);

  // MPU6050の存在確認
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(WHO_AM_I);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 1);

  if (Wire.available()) {
    byte whoami = Wire.read();
    Serial.print("WHO_AM_I: 0x");
    Serial.println(whoami, HEX);

    // 0x68=MPU6050, 0x70=MPU6050互換, 0x98=MPU6050, 0x71=MPU6500
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
  Wire.write(0x00);  // スリープ解除
  Wire.endTransmission();

  Serial.println("Calibrating... Keep still!");
  delay(1000);

  prevTime = millis();
  Serial.println("GO!");
}

void loop() {
  // 加速度・ジャイロデータ読み取り（14バイト）
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

  // 時間計算
  unsigned long currentTime = millis();
  float dt = (currentTime - prevTime) / 1000.0;
  prevTime = currentTime;

  // 加速度から角度計算（度）
  float accelAngle = atan2(ay, az) * 180.0 / PI;

  // ジャイロから角速度（度/秒）- 感度131 LSB/(°/s)
  float gyroRate = gx / 131.0;

  // 相補フィルタ
  angle = ALPHA * (angle + gyroRate * dt) + (1.0 - ALPHA) * accelAngle;

  // 結果表示
  Serial.print("Angle: ");
  Serial.print(angle, 1);
  Serial.print(" deg | Accel: ");
  Serial.print(accelAngle, 1);
  Serial.print(" | Gyro: ");
  Serial.print(gyroRate, 1);
  Serial.println(" deg/s");

  delay(20);  // 50Hz
}
