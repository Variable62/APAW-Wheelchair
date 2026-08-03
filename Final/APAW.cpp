#include <Wire.h>
// Input pin
const int MUX_SIG            = A0;   
const int Pressure_Sensor    = A2;  
// Output pin
const int MUX_S0             = 8;  
const int MUX_S1             = 9;    
const int MUX_S2             = 10;  
const int MUX_S3             = 11;  
const int RelayCh1_Airpump   = 3;    
const int RelayCh1_Valve     = 4;    
const int Buzzer             = 7;    

// Condition variables
const float TargetPressure   = 3.0; 
const float PressureMaxPsi   = 5.0;   
const float PressureMinPsi   = 0.0;
const int adcMin             = 102;
const int adcMax             = 921; 

int     fsrValues[6]         = {0, 0, 0, 0, 0, 0};
float   fsrMmHg[6]           = {0.0,0.0,0.0,0.0,0.0,0.0};

int     maxFsrValue          = 0;
float   pressurePsi          = 0.0;
float   maxFsrMmHg           = 0.0; 

unsigned long sittingStartTime       = 0;
unsigned long sittingDurationMinutes = 0;
bool          Sitting                 = false;

unsigned long previousMillis  = 0;
const long    interval        = 1000;  
unsigned long buzzerMillis    = 0;
bool          buzzerState     = false;

unsigned long dangerActionMillis = 0;
int           dangerStep         = 0; 
//-----------------------Constant value--------------
const float PressureThreshold = 32.0; //mmHg
const unsigned long WarningTime = 1; //minute
const unsigned long DangerTime = 4; //minute
const unsigned long BuzzerInterval = 500;

const char* currentStateStr = "IDLE";

float   fsr1 = 0.0, fsr2 = 0.0, fsr3 = 0.0, fsr4 = 0.0, fsr5 = 0.0, fsr6 = 0.0;
bool    pump_status = false;
bool    valve_status = false;

enum SystemState {
    StateIdle,
    StateNormal,
    StateWarning,
    StateDanger
};

SystemState CurrentState = StateIdle; 
SystemState LastState    = StateIdle; 

void SelectMUXCh(uint8_t ch){
    digitalWrite(MUX_S0, bitRead(ch, 0));
    digitalWrite(MUX_S1, bitRead(ch, 1));
    digitalWrite(MUX_S2, bitRead(ch, 2));
    digitalWrite(MUX_S3, bitRead(ch, 3));
    delayMicroseconds(100);
}

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

void ReadFSRSensorWithMux()
{
    maxFsrValue = 0;

    for (uint8_t i = 0; i < 6; i++)
    {
        SelectMUXCh(i);

        delayMicroseconds(500);
        analogRead(MUX_SIG);          // Dummy Read
        delayMicroseconds(500);

        fsrValues[i] = AverageAnalogRead(MUX_SIG);

        fsrMmHg[i] = (fsrValues[i] / 1023.0) * 120.0;

        if (fsrValues[i] > maxFsrValue)
        {
            maxFsrValue = fsrValues[i];
        }
    }

    maxFsrMmHg = (maxFsrValue / 1023.0) * 120.0;
}

void ReadPressureSensor(){
    float RawPressureSensor = AverageAnalogRead(Pressure_Sensor);
    int constrainedValue = constrain(RawPressureSensor, adcMin, adcMax);
    
    pressurePsi = PressureMinPsi + ((float)(constrainedValue - adcMin) / (float)(adcMax - adcMin)) * (PressureMaxPsi - PressureMinPsi);
}

void TrackSittingTime(){
    if (maxFsrMmHg >= PressureThreshold) { 
        if (!Sitting) {
            sittingStartTime = millis(); 
            Sitting = true;
        }
        sittingDurationMinutes = (millis() - sittingStartTime) / 60000; 
    } else {
        Sitting = false;
        sittingDurationMinutes = 0; 
    }
}

void CheckStateAlarm(){

    if (CurrentState == StateWarning) {
        currentStateStr = "WARNING";

        if (millis() - buzzerMillis >= BuzzerInterval) { 
            buzzerMillis = millis();
            buzzerState = !buzzerState;
            if (buzzerState) {
                digitalWrite(Buzzer, LOW);
                } else {
                digitalWrite(Buzzer, HIGH);    
            }
        }
    } 
    else if (CurrentState == StateDanger) {
        currentStateStr = "DANGER";
        digitalWrite(Buzzer, LOW); 
    }
    else if (CurrentState == StateNormal) {
        currentStateStr = "NORMAL";
        digitalWrite(Buzzer, HIGH);   
    }
    else { 
        currentStateStr = "IDLE";
        digitalWrite(Buzzer, HIGH);     
    }   
}

void AllSensorOff(){
    digitalWrite(RelayCh1_Airpump, HIGH); 
    digitalWrite(RelayCh1_Valve, HIGH);    
    digitalWrite(Buzzer, HIGH);             
}

void printDebugInfo()
{
    Serial.print("[mmHg] ");

    for (int i = 0; i < 6; i++)
    {
        Serial.print("FSR");
        Serial.print(i + 1);
        Serial.print(":");
        Serial.print(fsrMmHg[i], 1);

        if (i < 5) Serial.print("\t");
    }

    Serial.println();

    Serial.print("Pressure : ");
    Serial.print(pressurePsi, 2);
    Serial.print(" PSI");

    Serial.print(" | Sitting : ");
    Serial.print(sittingDurationMinutes);
    Serial.print(" min");

    Serial.print(" | Pump : ");
    Serial.print(pump_status ? "ON" : "OFF");

    Serial.print(" | Valve : ");
    Serial.print(valve_status ? "ON" : "OFF");

    Serial.print(" | State : ");
    Serial.println(currentStateStr);

    Serial.println("------------------------------------------------------------------------------------------------");
}

void UpdateSensorData()
{
    fsr1 = (fsrValues[0] / 1023.0) * 120.0;
    fsr2 = (fsrValues[1] / 1023.0) * 120.0;
    fsr3 = (fsrValues[2] / 1023.0) * 120.0;
    fsr4 = (fsrValues[3] / 1023.0) * 120.0;
    fsr5 = (fsrValues[4] / 1023.0) * 120.0;
    fsr6 = (fsrValues[5] / 1023.0) * 120.0;

}

void setup() {
    Serial.begin(115200);
    delay(1500); 

    pinMode(Pressure_Sensor, INPUT);
    pinMode(MUX_SIG, INPUT);        
    pinMode(MUX_S0, OUTPUT);
    pinMode(MUX_S1, OUTPUT);
    pinMode(MUX_S2, OUTPUT);
    pinMode(MUX_S3, OUTPUT);    
    pinMode(RelayCh1_Airpump , OUTPUT);
    pinMode(RelayCh1_Valve , OUTPUT);    
    pinMode(Buzzer, OUTPUT);

    AllSensorOff(); 
    Serial.println("System Ready");
}

void loop() {

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        ReadFSRSensorWithMux();
        ReadPressureSensor();
        TrackSittingTime();

        if (maxFsrMmHg < 70) { 
            CurrentState = StateIdle;
        }

        else if (maxFsrMmHg >=  PressureThreshold && sittingDurationMinutes >= DangerTime) {
            if (CurrentState != StateDanger) {
                CurrentState = StateDanger;
                dangerStep = 0; 
                dangerActionMillis = millis();
            }
        }
        else if (maxFsrMmHg >= PressureThreshold && sittingDurationMinutes >= WarningTime) {
            CurrentState = StateWarning;
        }
        else {
            CurrentState = StateNormal;
        }

        CheckStateAlarm();
        
        if (CurrentState != LastState) {
            UpdateSensorData();
            LastState = CurrentState;
        }

        switch(CurrentState){
            case StateIdle:
                AllSensorOff(); 
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
                    digitalWrite(RelayCh1_Airpump, LOW);     // Pump OFF
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
        }
                printDebugInfo();
    }
}