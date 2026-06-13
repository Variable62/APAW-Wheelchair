#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// Create MPU6050 object
Adafruit_MPU6050 mpu;

// Variables for time management (Non-blocking timing)
unsigned long previousMillis = 0;
const long interval = 100; // Sampling interval in milliseconds

void setup() {
  // Initialize Serial communication at 115200 bps
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect (required for Uno R4 native USB)
  }

  Serial.println("Initializing MPU6050...");

  // Try to initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip! Please check your wiring.");
    while (1) {
      delay(10); // Halt execution if sensor is not found
    }
  }
  Serial.println("MPU6050 Found and Initialized successfully!");

  // Set sensor ranges for standard operation
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
  // Get the current timestamp
  unsigned long currentMillis = millis();

  // Check if the specified interval has elapsed
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Save the last update time

    // Get new sensor events with the readings
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Calculate basic Tilt/Angle for X-axis (Roll) and Y-axis (Pitch) from accelerometer data
    float angleX = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;
    float angleY = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;

    // Output the raw calculated angles to the Serial Monitor without any condition
    Serial.print("Tilt Angle X: ");
    Serial.print(angleX, 1);
    Serial.print("° | Tilt Angle Y: ");
    Serial.print(angleY, 1);
    Serial.println("°");
  }
}
