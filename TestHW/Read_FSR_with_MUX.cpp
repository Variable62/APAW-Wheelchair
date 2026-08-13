const int MUX_SIG = A0;   
const int MUX_S0  = 8;    
const int MUX_S1  = 9;    
const int MUX_S2  = 10;   
const int MUX_S3  = 11;   

int fsrValues[6] = {0, 0, 0, 0, 0, 0}; 
float fsrMmHg[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; 

unsigned long previousMillis = 0;
const long interval = 100; 

// ===== Average Analog Read =====
int AverageAnalogRead(uint8_t pin, uint8_t samples = 5)
{
    long sum = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += analogRead(pin);
    }

    return sum / samples;
}

void SelectMUXCh(uint8_t ch){
    digitalWrite(MUX_S0, bitRead(ch, 0));
    digitalWrite(MUX_S1, bitRead(ch, 1));
    digitalWrite(MUX_S2, bitRead(ch, 2));
    digitalWrite(MUX_S3, bitRead(ch, 3));
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
      SelectMUXCh(i);     
      analogRead(MUX_SIG);      // discard
      delayMicroseconds(50);       
      fsrValues[i] = AverageAnalogRead(MUX_SIG); 
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
