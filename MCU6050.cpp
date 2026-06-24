#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(10);
  }

  if (!mpu.begin()) {
    Serial.println("MPU6050 not found!");
    while (1);
  }

  Serial.println("MPU6050 Ready");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;

  // คำนวณมุมเอียง
  float angleX = atan2(ay, az) * 180.0 / PI;
  float angleY = atan2(-ax, sqrt(ay * ay + az * az)) * 180.0 / PI;

  Serial.print("X: ");
  Serial.print(angleX);
  Serial.print(" deg");

  Serial.print("   Y: ");
  Serial.print(angleY);
  Serial.println(" deg");

  delay(100);
}
