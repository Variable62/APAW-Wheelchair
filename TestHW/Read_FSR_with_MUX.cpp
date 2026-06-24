const int MUX_SIG = A0;   
const int MUX_S0  = 8;    
const int MUX_S1  = 9;    
const int MUX_S2  = 10;   
const int MUX_S3  = 11;   

int fsrValues[6] = {0, 0, 0, 0, 0, 0}; 
float fsrMmHg[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; 

unsigned long previousMillis = 0;
const long interval = 100; 

void selectMuxChannel(int channel) {
  digitalWrite(MUX_S0, (channel & 1));
  digitalWrite(MUX_S1, (channel & 2));
  digitalWrite(MUX_S2, (channel & 4));
  digitalWrite(MUX_S3, (channel & 8));
  delayMicroseconds(100); 
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; } 

  pinMode(MUX_S0, OUTPUT);
  pinMode(MUX_S1, OUTPUT);
  pinMode(MUX_S2, OUTPUT);
  pinMode(MUX_S3, OUTPUT);

  Serial.println("Local MUX FSR Test Initialized with mmHg Mapping.");
  Serial.println("=================================================");
}

void loop() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    for (int i = 0; i < 6; i++) {
      selectMuxChannel(i);                
      fsrValues[i] = analogRead(MUX_SIG); 
      fsrMmHg[i] = (fsrValues[i] / 1023.0) * 120.0;
    }

    Serial.print("[RAW] ");
    for (int i = 0; i < 6; i++) {
      Serial.print("FSR"); Serial.print(i); Serial.print(":");
      Serial.print(fsrValues[i]);
      if (i < 5) Serial.print("\t");
    }
    
    Serial.print("  ||  [mmHg] ");
    for (int i = 0; i < 6; i++) {
      Serial.print("FSR"); Serial.print(i); Serial.print(":");
      Serial.print(fsrMmHg[i], 1); 
      if (i < 5) Serial.print("\t");
    }
    Serial.println(); 
  }
}
