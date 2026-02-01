// ESP32 Lチカ (LED Blink)
// 内蔵LEDを点滅させる

#define LED_PIN 2  // ESP32内蔵LED（GPIO2）

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("ESP32 Blink Start!");
}

void loop() {
  digitalWrite(LED_PIN, HIGH);
  Serial.println("ON");
  delay(500);

  digitalWrite(LED_PIN, LOW);
  Serial.println("OFF");
  delay(500);
}
