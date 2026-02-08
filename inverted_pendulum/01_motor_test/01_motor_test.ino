/*
 * モーター動作確認テスト
 * ESP32 + DRV8833
 *
 * ESP32 Arduino Core 3.x対応版
 *
 * 接続:
 *   GPIO16 → AIN1 (モーターA)
 *   GPIO17 → AIN2
 *   GPIO18 → BIN1 (モーターB)
 *   GPIO19 → BIN2
 *   3.3V   → STBY (スタンバイ解除)
 */

// ピン定義
#define AIN1 16
#define AIN2 17
#define BIN1 18
#define BIN2 19

// PWM設定
#define PWM_FREQ 1000      // 1kHz
#define PWM_RESOLUTION 8   // 8bit (0-255)

void setup() {
  Serial.begin(115200);
  Serial.println("Motor Test Start!");

  // ESP32 Arduino Core 3.x: ledcAttach(pin, freq, resolution)
  // チャンネルは自動割り当て、ピン番号で直接制御
  ledcAttach(AIN1, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(AIN2, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(BIN1, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(BIN2, PWM_FREQ, PWM_RESOLUTION);

  // 初期状態：停止
  stopMotors();

  delay(2000);
}

void loop() {
  // テスト1: 両モーター前進
  Serial.println(">> Forward");
  setMotors(150, 150);
  delay(2000);

  // 停止
  Serial.println(">> Stop");
  stopMotors();
  delay(1000);

  // テスト2: 両モーター後退
  Serial.println(">> Backward");
  setMotors(-150, -150);
  delay(2000);

  // 停止
  Serial.println(">> Stop");
  stopMotors();
  delay(1000);

  // テスト3: 右旋回（左前進、右後退）
  Serial.println(">> Turn Right");
  setMotors(150, -150);
  delay(2000);

  // 停止
  Serial.println(">> Stop");
  stopMotors();
  delay(1000);

  // テスト4: 左旋回（左後退、右前進）
  Serial.println(">> Turn Left");
  setMotors(-150, 150);
  delay(2000);

  // 停止
  Serial.println(">> Stop");
  stopMotors();
  delay(3000);
}

/**
 * 両モーターの速度を設定
 * speedA, speedB: -255 ~ 255 (負で逆転)
 */
void setMotors(int speedA, int speedB) {
  // モーターA (ESP32 Core 3.x: ledcWrite(pin, duty))
  if (speedA >= 0) {
    ledcWrite(AIN1, speedA);
    ledcWrite(AIN2, 0);
  } else {
    ledcWrite(AIN1, 0);
    ledcWrite(AIN2, -speedA);
  }

  // モーターB
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
