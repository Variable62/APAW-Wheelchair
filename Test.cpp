#include <Wire.h>

const int MUX_SIG            = A0;   
const int MUX_S0             = 8;    
const int MUX_S1             = 9;   
const int MUX_S2             = 10;  
const int MUX_S3             = 11;  
const int Pressure_Sensor    = A2;   
const int RelayCh1_Airpump   = 3;    
const int RelayCh1_Valve     = 4;   
const int Buzzer             = 2;    

const float PressureMaxPsi   = 5.0;   
const float PressureMinPsi   = 0.0;
const int adcMin             = 102;
const int adcMax             = 921; 

int     fsrValues[6]         = {0, 0, 0, 0, 0, 0};
int     maxFsrValue          = 0;
float   pressurePsi          = 0.0;

unsigned long sittingStartTime = 0;
unsigned long sittingDurationMinutes = 0;
bool            Siting            = false;

unsigned long   previousMillis  = 0;
const long      interval        = 100;  
unsigned long   buzzerMillis    = 0;
bool            buzzerState     = false;

unsigned long   dangerActionMillis = 0;
int             dangerStep         = 0; 

enum SystemState {
    StateIdle,
    StateNormal,
    StateWarning,
    StateDanger
};

SystemState CurrentState = StateIdle; 

void SelectMUXCh(int CH){
    digitalWrite(MUX_S0 , (CH & 1));
    digitalWrite(MUX_S1 , (CH & 2));
    digitalWrite(MUX_S2 , (CH & 4));
    digitalWrite(MUX_S3 , (CH & 8));
    delayMicroseconds(20); 
}

void ReadFSRSensorWithMux(){
    maxFsrValue = 0;
    for (int i = 0; i < 6; i++) {
        SelectMUXCh(i);
        fsrValues[i] = analogRead(MUX_SIG); 
        if (fsrValues[i] > maxFsrValue) {
            maxFsrValue = fsrValues[i];
        }
    }
}

void ReadPressureSensor(){
    float RawPressureSensor = analogRead(Pressure_Sensor);
    int constrainedValue = constrain(RawPressureSensor, adcMin, adcMax);
    pressurePsi = map(constrainedValue, adcMin, adcMax, PressureMinPsi, PressureMaxPsi);
}

void TrackSittingTime(){
    if (maxFsrValue > 200) { 
        if (!Siting) {
            sittingStartTime = millis(); 
            Siting = true;
        }
        sittingDurationMinutes = (millis() - sittingStartTime) / 60000; 
    } else {
        Siting = false;
        sittingDurationMinutes = 0; 
    }
}

void allsensor_off(){
    digitalWrite(RelayCh1_Airpump, HIGH); 
    digitalWrite(RelayCh1_Valve, HIGH);    
    digitalWrite(Buzzer, LOW);            
}

void printDebugInfo() {
    Serial.print("[TIME] Current: ");
    Serial.print(millis() / 1000.0, 1);
    Serial.print("s | Start at: ");
    Serial.print(sittingStartTime / 1000.0, 1);
    Serial.print("s | Duration: ");
    Serial.print(sittingDurationMinutes);
    Serial.print(" min ");
    
    Serial.print("|| [SENSOR] Max FSR: ");
    Serial.print(maxFsrValue);
    Serial.print(" | Pressure: ");
    Serial.print(pressurePsi, 2);
    Serial.print(" PSI || State: ");
    
    switch(CurrentState) {
        case StateIdle:    Serial.println("IDLE"); break;
        case StateNormal:  Serial.println("NORMAL"); break;
        case StateWarning: Serial.println("WARNING"); break;
        case StateDanger:  Serial.print("DANGER (Step "); Serial.print(dangerStep); Serial.println(")"); break;
    }
}

void setup() {
    Serial.begin(115200);

    pinMode(Pressure_Sensor, INPUT);
    pinMode(MUX_SIG, INPUT);        
    pinMode(MUX_S0, OUTPUT);
    pinMode(MUX_S1, OUTPUT);
    pinMode(MUX_S2, OUTPUT);
    pinMode(MUX_S3, OUTPUT);    
    pinMode(RelayCh1_Airpump , OUTPUT);
    pinMode(RelayCh1_Valve , OUTPUT);    
    pinMode(Buzzer, OUTPUT);

    allsensor_off(); 
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        ReadFSRSensorWithMux();
        ReadPressureSensor();
        TrackSittingTime();

        if (maxFsrValue > 800 && sittingDurationMinutes >= 120) {
            if (CurrentState != StateDanger) {
                CurrentState = StateDanger;
                dangerStep = 0; 
                dangerActionMillis = millis();
            }
        }
        else if (maxFsrValue > 200 && sittingDurationMinutes >= 30) {
            CurrentState = StateWarning;
        }
        else if (maxFsrValue > 200 && sittingDurationMinutes < 30) {
            CurrentState = StateNormal;
        }
        else {
            CurrentState = StateIdle;
        }

        printDebugInfo();

        switch(CurrentState){
            case StateIdle:
                allsensor_off(); 
                break;

            case StateNormal:
                digitalWrite(RelayCh1_Airpump, HIGH);
                digitalWrite(RelayCh1_Valve, HIGH);   
                break;

            case StateWarning:
                if (pressurePsi < 1.0) { 
                    digitalWrite(RelayCh1_Airpump, LOW); 
                } 
                else if (pressurePsi >= 1.4) {
                    digitalWrite(RelayCh1_Airpump, HIGH); 
                }
                digitalWrite(RelayCh1_Valve, HIGH); 
                break;

            case StateDanger:
                if (dangerStep == 0) {
                    digitalWrite(RelayCh1_Valve, LOW);   
                    digitalWrite(RelayCh1_Airpump, HIGH); 
                    
                    if (millis() - dangerActionMillis >= 5000 || pressurePsi <= 0.5) {
                        dangerStep = 1; 
                        dangerActionMillis = millis(); 
                    }
                } 
                else if (dangerStep == 1) {
                    digitalWrite(RelayCh1_Valve, HIGH);   
                    digitalWrite(RelayCh1_Airpump, LOW);  
                    
                    if (pressurePsi >= 1.3) {
                        dangerStep = 0; 
                        dangerActionMillis = millis();
                    }
                }
                break;
        }
    }
}
