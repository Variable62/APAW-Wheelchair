/*
  FSR 6-Channel Reading Code for Arduino Uno R4
  Hardware setup: FSR with 10k Ohm Voltage Divider connected to A0 - A5
*/

// Define analog input pins for the 6 FSR sensors
const int fsrPins[6] = {A0, A1, A2, A3, A4, A5};

// Array to store raw ADC values (0 - 1023)
int fsrRawValues[6] = {0, 0, 0, 0, 0, 0};

// Variables for time management (Non-blocking timing)
unsigned long previousMillis = 0;
const long interval = 200; // Sampling and display interval in milliseconds

void setup() {
  // Initialize Serial communication at 115200 bps
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect (required for Uno R4 native USB)
  }
  
  Serial.println("--- FSR 6-Channel Reading Initialized ---");
}

void loop() {
  // Get the current timestamp
  unsigned long currentMillis = millis();

  // Check if the specified interval has elapsed
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; // Save the last update time

    // Loop through and read all 6 analog channels
    for (int i = 0; i < 6; i++) {
      fsrRawValues[i] = analogRead(fsrPins[i]);
    }

    // Output the data to the Serial Monitor
    printFsrData();
  }
}

// Function to format and print FSR data
void printFsrData() {
  Serial.print("FSR Values -> ");
  for (int i = 0; i < 6; i++) {
    Serial.print("CH");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(fsrRawValues[i]);
    
    // Print separator between channels except for the last one
    if (i < 5) {
      Serial.print(" | ");
    }
  }
  Serial.println(); // Print a new line
}
