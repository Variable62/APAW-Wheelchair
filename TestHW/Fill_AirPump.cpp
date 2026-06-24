const int Pressure_Sensor    = A2;   
const int RelayCh1_Airpump   = 3;    
const int RelayCh1_Valve     = 4;    

const float PressureMaxPsi   = 10.0;  
const float PressureMinPsi   = 0.0;
const int adcMin             = 102;   
const int adcMax             = 921;   

const float TargetPressure   = 1;  //Magic number  

float pressurePsi = 0.0;
unsigned long previousMillis = 0;
const long interval = 200;            

void setup() {
  Serial.begin(115200);

  pinMode(Pressure_Sensor, INPUT);
  pinMode(RelayCh1_Airpump, OUTPUT);
  pinMode(RelayCh1_Valve, OUTPUT);

  digitalWrite(RelayCh1_Airpump, HIGH); 
  digitalWrite(RelayCh1_Valve, HIGH);    
}

void loop() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float RawPressureSensor = analogRead(Pressure_Sensor);
    int constrainedValue = constrain(RawPressureSensor, adcMin, adcMax);
    pressurePsi = map(constrainedValue, adcMin, adcMax, PressureMinPsi, PressureMaxPsi);

    Serial.print("Pressure: ");
    Serial.print(pressurePsi);
    Serial.println(" PSI");

    if (pressurePsi < TargetPressure) {
      digitalWrite(RelayCh1_Airpump, LOW);  
      digitalWrite(RelayCh1_Valve, HIGH);   
    } 
    else {
      digitalWrite(RelayCh1_Airpump, HIGH); 
      digitalWrite(RelayCh1_Valve, HIGH);   
    }
  }
}
