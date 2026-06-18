#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

unsigned long previousMillis = 0;
const long interval = 100; 
bool mpuAvailable = false; 

void setup() {
  Serial.begin(115200);
  
  unsigned long startTime = millis();
  while (!Serial && (millis() - startTime < 4000)) {
    ; 
  }

  Serial.println("\n--- MPU6050 Advanced Test on Uno R4 ---");
  Serial.println("Attempting to connect to MPU6050...");
  
  if (!mpu.begin(0x68, &Wire)) { 
      Serial.println("[ERROR] Cannot find MPU6050 chip!");
      Serial.println("👉 แนะนำให้ลองย้ายสาย SDA/SCL ไปพินมุมบนสุด แล้วแก้โค้ดบรรทัดด้านบนเป็น &Wire1");
      mpuAvailable = false;
  } else {
      Serial.println("[SUCCESS] MPU6050 Found and Initialized successfully!");
      mpuAvailable = true;

      mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
      mpu.setGyroRange(MPU6050_RANGE_250_DEG);
      mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 

    if (!mpuAvailable) {
      Serial.println("[STATUS] MPU6050 Disconnected... [Waiting for hardware fix]");
      return; 
    }

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
