const int relayCh1Pin = 2;
const int relayCh2Pin = 3;

const int RELAY_ON = LOW;
const int RELAY_OFF = HIGH;


bool toggleState = false;

unsigned long previousMillis = 0;
const long interval = 2000; 

void setup() {
  
  Serial.begin(115200);
  while (!){
    ;
  }
  pinMode(relayCh1Pin, OUTPUT);
  pinMode(relayCh2Pin, OUTPUT);
  
  digitalWrite(relayCh1Pin, RELAY_OFF);
  digitalWrite(relayCh2Pin, RELAY_OFF);

  Serial.println("--- 2-Channel Relay Control Initialized ---");
}

void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    toggleState = !toggleState;

    if (toggleState) {
  
      digitalWrite(relayCh1Pin, RELAY_ON);
      digitalWrite(relayCh2Pin, RELAY_OFF);
      Serial.println("Relay Status -> CH1: ON  | CH2: OFF");
    } else {
      
      digitalWrite(relayCh1Pin, RELAY_OFF);
      digitalWrite(relayCh2Pin, RELAY_ON);
      Serial.println("Relay Status -> CH1: OFF | CH2: ON");
    }
  }
}
