#include "thingProperties.h"
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

const float TargetPressure = 3.0; 

const float PressureMaxPsi   = 5.0;   
const float PressureMinPsi   = 0.0;
const int adcMin             = 102;
const int adcMax             = 921; 

int     fsrValues[6]         = {0, 0, 0, 0, 0, 0};
int     maxFsrValue          = 0;
float   pressurePsi          = 0.0;
float   maxFsrMmHg           = 0.0; 

int16_t AcX, AcY, AcZ;
float   angleX               = 0.0;
float   angleY               = 0.0;

unsigned long   sittingStartTime = 0;
unsigned long   sittingDurationMinutes = 0;
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
    StateDanger,
    StateTiltWarning
};

SystemState CurrentState = StateIdle; 
SystemState LastState    = StateIdle; 

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

void ReadMPU6050Angle(){
    Wire.beginTransmission(0x68);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(0x68, 6, true);

    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();

    angleX = atan2(AcY, AcZ) * 180.0 / PI;
    angleY = atan2(-AcX, sqrt((long)AcY * AcY + (long)AcZ * AcZ)) * 180.0 / PI;
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
    if (CurrentState == StateTiltWarning) {
        led_danger  = true;
        led_normal  = false;
        led_warning = true;
        state       = "TILT_WARNING";
        tone(Buzzer, 10000); 
    }
    else if (CurrentState == StateWarning) {
        led_warning = true;
        led_normal  = false;
        led_danger  = false;
        state       = "WARNING";

        if (millis() - buzzerMillis >= 500) { 
            buzzerMillis = millis();
            buzzerState = !buzzerState;
            if (buzzerState) {
                tone(Buzzer, 2000); 
            } else {
                noTone(Buzzer);     
            }
        }
    } 
    else if (CurrentState == StateDanger) {
        led_danger  = true;
        led_normal  = false;
        led_warning = false;
        state       = "DANGER";
        tone(Buzzer, 2500); 
    }
    else if (CurrentState == StateNormal) {
        led_normal  = true;
        led_warning = false;
        led_danger  = false;
        state       = "NORMAL";
        noTone(Buzzer);     
    }
    else { 
        led_normal  = false;
        led_warning = false;
        led_danger  = false;
        state       = "IDLE";
        noTone(Buzzer);     
    }
}

void allsensor_off(){
    digitalWrite(RelayCh1_Airpump, HIGH); 
    digitalWrite(RelayCh1_Valve, HIGH);    
    noTone(Buzzer);            
}

void printDebugInfo() {
    Serial.print("[FSR Raw Values] ");
    for(int i = 0; i < 6; i++) {
        Serial.print("CH"); Serial.print(i+1); Serial.print(":"); Serial.print(fsrValues[i]);
        if(i < 5) Serial.print(" | ");
    }
    Serial.println();

    Serial.print(" -> [SUMMARY] Max FSR: "); Serial.print(maxFsrValue);
    Serial.print(" ("); Serial.print(maxFsrMmHg, 1); Serial.print(" mmHg)");
    
    Serial.print(" || Air Pressure: "); Serial.print(pressurePsi, 2); Serial.print(" PSI");
    Serial.print(" || Duration: "); Serial.print(sittingDurationMinutes); Serial.print(" min");
    
    Serial.print(" || MPU6050 -> X: "); Serial.print(angleX, 1); 
    Serial.print(" deg | Y: "); Serial.print(angleY, 1); Serial.print(" deg");
    
    Serial.print(" || PUMP: "); Serial.print(digitalRead(RelayCh1_Airpump) == LOW ? "ON" : "OFF");
    Serial.print(" | VALVE: "); Serial.print(digitalRead(RelayCh1_Valve) == LOW ? "ON" : "OFF");
    Serial.print(" | BUZZER: "); Serial.print(digitalRead(Buzzer) == HIGH ? "ON" : "OFF");
    Serial.print(" || SYSTEM STATE: "); Serial.println(state);
    Serial.println("------------------------------------------------------------------------------------------------------------------------");
}

void setup() {
    Serial.begin(115200);
    delay(1500); 

    Wire.begin();
    Wire.beginTransmission(0x68);
    Wire.write(0x6B);
    Wire.write(0);
    Wire.endTransmission(true);
    Serial.println("MPU6050 Ready");

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
        ReadMPU6050Angle();
        TrackSittingTime();

        if (maxFsrValue < 100) { 
            CurrentState = StateIdle;
        }
        else if (abs(angleX) > 30.0 || abs(angleY) > 30.0) {
            CurrentState = StateTiltWarning;
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
        printDebugInfo();
        if (CurrentState != LastState) {
            fsr1 = (fsrValues[0] / 1023.0) * 120.0;
            fsr2 = (fsrValues[1] / 1023.0) * 120.0;
            fsr3 = (fsrValues[2] / 1023.0) * 120.0;
            fsr4 = (fsrValues[3] / 1023.0) * 120.0;
            fsr5 = (fsrValues[4] / 1023.0) * 120.0;
            fsr6 = (fsrValues[5] / 1023.0) * 120.0;
            Gyx  = angleX;
            GyY  = angleY;
            
            LastState = CurrentState;
        }

        switch(CurrentState){
            case StateIdle:
                allsensor_off(); 
                pump_status = false;
                valve_status = false;
                break;

            case StateNormal:
                digitalWrite(RelayCh1_Airpump, HIGH);
                digitalWrite(RelayCh1_Valve, HIGH);   
                pump_status = false;
                valve_status = false;
                break;

            case StateWarning:

            if (pressurePsi < TargetPressure) {
                digitalWrite(RelayCh1_Airpump, LOW);      // Pump ON
                pump_status = true;
            }
            else {
                digitalWrite(RelayCh1_Airpump, HIGH);     // Pump OFF
                pump_status = false;
            }

            digitalWrite(RelayCh1_Valve, HIGH);           // Valve OFF
            valve_status = false;

            break;

            case StateDanger:
                if (dangerStep == 0) {
                    digitalWrite(RelayCh1_Valve, LOW);   
                    digitalWrite(RelayCh1_Airpump, HIGH); 
                    valve_status = true;
                    pump_status = false;
                    if (millis() - dangerActionMillis >= 5000) {
                        dangerStep = 1; 
                        dangerActionMillis = millis(); 
                    }
                } 
                else if (dangerStep == 1) {

                    digitalWrite(RelayCh1_Valve, HIGH);      // Valve OFF
                    valve_status = false;

                    if (pressurePsi < TargetPressure) {

                    digitalWrite(RelayCh1_Airpump, LOW); // Pump ON
                    pump_status = true;
                }
                else {

                    digitalWrite(RelayCh1_Airpump, HIGH); // Pump OFF
                    pump_status = false;

                    dangerStep = 0;
                    dangerActionMillis = millis();
                }
            }
                break;

            case StateTiltWarning:
                digitalWrite(RelayCh1_Airpump, HIGH);
                digitalWrite(RelayCh1_Valve, HIGH);   
                pump_status = false;
                valve_status = false;
                break;
        }
    }
}

void onLedDangerChange() {}
void onLedNormalChange() {}
void onLedWarningChange() {}
void onPumpStatusChange() {}
void onValveStatusChange() {}
void onStateChange() {}
void onFsr1Change() {}
void onFsr2Change() {}
void onFsr3Change() {}
void onFsr4Change() {}
void onFsr5Change() {}
void onFsr6Change() {}