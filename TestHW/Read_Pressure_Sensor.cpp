const int pressurePin = A2;

const float sensorMaxPsi = 150.0; 
const float sensorMinPsi = 0.0;   

const int adcMin = 102; 
const int adcMax = 92;


unsigned long previousMillis = 0;
const long interval = 500; 

void setup() {

  Serial.begin(115200);
  while (!Serial) {
;
  }
  
  Serial.println("--- Pressure Sensor Reading Initialized ---");
}

void loop() {

  unsigned long currentMillis = millis();


  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis; 

    int rawValue = analogRead(pressurePin);

    float voltage = (rawValue * 5.0) / 1023.0;

    int constrainedValue = constrain(rawValue, adcMin, adcMax);
    float pressurePsi = map(constrainedValue, adcMin, adcMax, sensorMinPsi, sensorMaxPsi);

    float pressureBar = pressurePsi * 0.0689476;

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
