// ESP32 Motor Control
// L293Dモータードライバで車を制御する

// ピン番号の設定
#define IN1 23
#define IN2 19  // ESP32ではGPIO24が使えないため19に変更
#define IN3 27
#define IN4 22

void setup() {
  // ピンを出力モードに設定
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // シリアル通信開始
  Serial.begin(115200);
  Serial.println("Motor control start!");

  // テスト走行
  forward();
  delay(2000);

  stop();
  delay(1000);

  backward();
  delay(2000);

  stop();
  delay(1000);

  turnLeft();
  delay(1000);

  stop();
  delay(1000);

  turnRight();
  delay(1000);

  stop();
  Serial.println("Done!");
}

void loop() {
  // テスト走行後は何もしない
}

// 前進
void forward() {
  Serial.println("Forward");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// 後退
void backward() {
  Serial.println("Backward");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// 停止
void stop() {
  Serial.println("Stop");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

// 左旋回
void turnLeft() {
  Serial.println("Turn Left");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

// 右旋回
void turnRight() {
  Serial.println("Turn Right");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}
