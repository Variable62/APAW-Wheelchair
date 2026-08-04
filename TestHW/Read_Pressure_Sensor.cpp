const int PressurePin = A4;
const float OffSet = 0.50;

void setup() {
  Serial.begin(115200);
  analogReadResolution(10);
}

void loop() {

  int adc = analogRead(PressurePin);
  float voltage = adc * 5.0 / 1023.0;

  float pressureMPa = (voltage - OffSet) * (1.6 / 4.0);
  
  if (pressureMPa < 0)
    pressureMPa = 0;

  float pressurePSI = pressureMPa * 145.038;

  Serial.print("ADC : ");
  Serial.print(adc);

  Serial.print("   Voltage : ");
  Serial.print(voltage, 3);
  Serial.print(" V");

  Serial.print("   PSI : ");
  Serial.println(pressurePSI, 2);

  delay(500);
}