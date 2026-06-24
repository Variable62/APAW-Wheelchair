const int fsrPins[6] = {A0, A1, A2, A3, A4, A5};
int fsrRawValues[6] = {0, 0, 0, 0, 0, 0};


unsigned long previousMillis = 0;
const long interval = 200;

void setup() {

  Serial.begin(115200);
  while (!Serial) {
;
  }
  
  Serial.println("--- FSR 6-Channel Reading Initialized ---");
}

void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    for (int i = 0; i < 6; i++) {
      fsrRawValues[i] = analogRead(fsrPins[i]);
    }
    printFsrData();
  }
}
void printFsrData() {
  Serial.print("FSR Values -> ");
  for (int i = 0; i < 6; i++) {
    Serial.print("CH");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(fsrRawValues[i]);
  
    if (i < 5) {
      Serial.print(" | ");
    }
  }
  Serial.println(); 
}
