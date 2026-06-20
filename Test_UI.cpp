#include "thingProperties.h"

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
float   maxFsrMmHg           = 0.0; 

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
    delayMicroseconds(100); 
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
    maxFsrMmHg = (maxFsrValue / 1023.0) * 120.0; 
}

void ReadPressureSensor(){
    float RawPressureSensor = analogRead(Pressure_Sensor);
    int constrainedValue = constrain(RawPressureSensor, adcMin, adcMax);
    pressurePsi = map(constrainedValue, adcMin, adcMax, PressureMinPsi, PressureMaxPsi);
}

void TrackSittingTime(){
    if (maxFsrMmHg >= 32.0) { 
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

void CheckStateAlarm(){
    if (CurrentState != StateIdle) {
        if (CurrentState == StateWarning) {
            if (millis() - buzzerMillis >= 500) { 
                buzzerMillis = millis();
                buzzerState = !buzzerState;
                digitalWrite(Buzzer, buzzerState); 
            }
        } 
        else if (CurrentState == StateDanger) {
            digitalWrite(Buzzer, HIGH); 
        }
        else {
            digitalWrite(Buzzer, LOW); 
        }
    } else {
        digitalWrite(Buzzer, LOW); 
    }
}

void allsensor_off(){
    digitalWrite(RelayCh1_Airpump, HIGH); 
    digitalWrite(RelayCh1_Valve, HIGH);    
    digitalWrite(Buzzer, LOW);            
}

void updateCloudVariables() {
    led_normal  = (CurrentState == StateNormal);
    led_warning = (CurrentState == StateWarning);
    led_danger  = (CurrentState == StateDanger);

    pump_status  = (digitalRead(RelayCh1_Airpump) == LOW); 
    valve_status = (digitalRead(RelayCh1_Valve) == LOW);
}

void setup() {
    Serial.begin(115200);
    delay(1500); 

    initProperties();
    ArduinoCloud.begin(ArduinoIoTPreferredConnection);
    setDebugMessageLevel(2);
    ArduinoCloud.printDebugInfo();

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
    ArduinoCloud.update();
    
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        ReadFSRSensorWithMux();
        ReadPressureSensor();
        TrackSittingTime();

        if (maxFsrValue < 100) { 
            CurrentState = StateIdle;
        }
        else if (maxFsrMmHg >= 32.0 && sittingDurationMinutes >= 150) {
            if (CurrentState != StateDanger) {
                CurrentState = StateDanger;
                dangerStep = 0; 
                dangerActionMillis = millis();
            }
        }
        else if (maxFsrMmHg >= 32.0 && sittingDurationMinutes >= 120) {
            CurrentState = StateWarning;
        }
        else {
            CurrentState = StateNormal;
        }

        CheckStateAlarm();
        updateCloudVariables();

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
