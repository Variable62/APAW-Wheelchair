#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(115200);

  Serial.println("Scanning...");

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);

    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
}

void loop() {}
