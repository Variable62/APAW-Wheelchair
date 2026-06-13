// Define the analog pin connected to the pressure sensor
const int pressurePin = A7;

// Sensor specifications (Adjust these based on your specific sensor datasheet)
const float sensorMaxPsi = 150.0; // Example: 150 PSI max range (Change to 30, 100, 200, etc. depending on your model)
const float sensorMinPsi = 0.0;   // Minimum pressure at 0.5V

// Calibration constants for a standard 0.5V - 4.5V sensor with 5V supply
// 0.5V from 5V ADC (10-bit: 0-1023) is approx 102
// 4.5V from 5V ADC (10-bit: 0-1023) is approx 921
const int adcMin = 102; // ADC value at 0.5V
const int adcMax = 921; // ADC value at 4.5V

// Variables for time management (Non-blocking timing)
unsigned long previousMillis = 0;
const long interval = 500; // Sampling and display interval in milliseconds

void setup() {
  // Initialize Serial communication at 115200 bps
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect (required for Uno R4 native USB)
  }
  
  Serial.println("--- Pressure Sensor Reading Initialized ---");
}

void loop() {
  // Get the current timestamp
  unsigned long currentMillis = millis();

  // Check if the specified interval has elapsed
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Save the last update time

    // Read the raw ADC value (0 - 1023)
    int rawValue = analogRead(pressurePin);

    // Calculate voltage for debugging/monitoring
    float voltage = (rawValue * 5.0) / 1023.0;

    // Map the ADC value to the actual pressure (PSI)
    // Constrain the raw value to prevent out-of-range calculation due to noise
    int constrainedValue = constrain(rawValue, adcMin, adcMax);
    float pressurePsi = map(constrainedValue, adcMin, adcMax, sensorMinPsi, sensorMaxPsi);

    // Convert PSI to Bar (1 PSI = 0.0689476 Bar) Optional
    float pressureBar = pressurePsi * 0.0689476;

    // Output the data to the Serial Monitor
    Serial.print("Raw ADC: ");
    Serial.print(rawValue);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 2);
    Serial.print("V | Pressure: ");
    Serial.print(pressurePsi, 1);
    Serial.print(" PSI (");
    Serial.print(pressureBar, 2);
    Serial.println(" Bar)");
  }
}
