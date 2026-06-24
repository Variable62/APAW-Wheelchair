#include <Wire.h>

int16_t AcX, AcY, AcZ;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("MPU6050 Ready");
}

void loop() {

  Wire.beginTransmission(0x68);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6, true);

  AcX = Wire.read() << 8 | Wire.read();
  AcY = Wire.read() << 8 | Wire.read();
  AcZ = Wire.read() << 8 | Wire.read();

  float angleX = atan2(AcY, AcZ) * 180.0 / PI;
  float angleY = atan2(-AcX, sqrt((long)AcY * AcY + (long)AcZ * AcZ)) * 180.0 / PI;

  Serial.print("Angle X: ");
  Serial.print(angleX, 1);
  Serial.print(" deg\t");

  Serial.print("Angle Y: ");
  Serial.print(angleY, 1);
  Serial.println(" deg");

  delay(100);
}
