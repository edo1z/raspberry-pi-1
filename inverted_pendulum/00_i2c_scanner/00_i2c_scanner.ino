/*
 * I2Cスキャナー
 * 接続されているI2Cデバイスのアドレスを検出
 */

#include <Wire.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nI2C Scanner Start!");

  // ESP32のI2Cピン: SDA=21, SCL=22
  Wire.begin(21, 22);
}

void loop() {
  byte error, address;
  int deviceCount = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println(" !");
      deviceCount++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (deviceCount == 0) {
    Serial.println("No I2C devices found!\n");
    Serial.println("Check wiring:");
    Serial.println("  SDA -> GPIO21");
    Serial.println("  SCL -> GPIO22");
    Serial.println("  VCC -> 3.3V");
    Serial.println("  GND -> GND");
  } else {
    Serial.print("Found ");
    Serial.print(deviceCount);
    Serial.println(" device(s)\n");
    Serial.println("MPU6050 should be at 0x68 or 0x69");
  }

  delay(3000);
}
