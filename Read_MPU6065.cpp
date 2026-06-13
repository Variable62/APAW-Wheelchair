#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>


Adafruit_MPU6050 mpu;


unsigned long previousMillis = 0;
const long interval = 100; 

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    ; 
  }

  Serial.println("Initializing MPU6050...");

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip! Please check your wiring.");
    while (1) {
      delay(10); 
    }
  }
  Serial.println("MPU6050 Found and Initialized successfully!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {

  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float angleX = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;
    float angleY = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;

    Serial.print("Tilt Angle X: ");
    Serial.print(angleX, 1);
    Serial.print("° | Tilt Angle Y: ");
    Serial.print(angleY, 1);
    Serial.println("°");
  }
}
