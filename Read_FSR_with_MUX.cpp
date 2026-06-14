const int MUX_SIG = A0;   // Main Analog read pin
const int MUX_S0  = 8;    // Control Pin S0
const int MUX_S1  = 9;    // Control Pin S1
const int MUX_S2  = 10;   // Control Pin S2
const int MUX_S3  = 11;   // Control Pin S3

int fsrValues[6] = {0, 0, 0, 0, 0, 0}; 

unsigned long previousMillis = 0;
const long interval = 100; 

void selectMuxChannel(int channel) {

  digitalWrite(MUX_S0, (channel & 1));
  digitalWrite(MUX_S1, (channel & 2));
  digitalWrite(MUX_S2, (channel & 4));
  digitalWrite(MUX_S3, (channel & 8));
  delayMicroseconds(20);
}

void setup() {

  Serial.begin(115200);
  while (!Serial) { ; } 

  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);

  Serial.println("Local MUX FSR Test Initialized.");
  Serial.println("Format: FSR0, FSR1, FSR2, FSR3, FSR4, FSR5");
  Serial.println("=========================================");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    for (int i = 0; i < 6; i++) {
      selectMuxChannel(i);                
      fsrValues[i] = analogRead(MUX_SIG); 
    }

    for (int i = 0; i < 6; i++) {
      Serial.print(fsrValues[i]);
      if (i < 5) {
        Serial.print("\t"); 
      }
    }
    Serial.println(); 
  }
}
