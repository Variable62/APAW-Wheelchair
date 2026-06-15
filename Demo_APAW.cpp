#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Adafruit_MPU6050 mpu;

//--------------------Assign Pins--------------------
const int MUX_SIG            = A0;   
const int MUX_C0             = 8;    
const int MUX_C1             = 9;   
const int MUX_C2             = 10;  
const int MUX_C3             = 11;  
const int Pressure_Sensor    = A2;   
const int RelayCh1_Airpump   = 3;    
const int RelayCh1_Valve     = 4;   
const int Buzzer             = 2;    

//---------------Limit Pressure wait raw data
const float PressureMaxPsi   = 150.0;
const float PressureMinPsi   = 0.0;
const int adcMin             = 102;
const int adcMax             = 921; 

int     fsrValues[6]         = {0, 0, 0, 0, 0, 0};
int     maxFsrValue          = 0;
float   pressurePsi          = 0.0;
float   angleX               = 0.0;
float   angleY               = 0.0;

unsigned long sittingStartTime = 0;
unsigned long sittingDurationMinutes = 0;
bool            Siting            = false;

unsigned long   previousMillis  = 0;
const long      interval        = 100;  
unsigned long   buzzerMillis    = 0;
bool            buzzerState     = false;

// ตัวแปรควบคุมเวลาสลับการทำงานใน StateDanger แบบไม่หน่วงบอร์ด
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
    digitalWrite(MUX_C0 , (CH & 1));
    digitalWrite(MUX_C1 , (CH & 2));
    digitalWrite(MUX_C2 , (CH & 4));
    digitalWrite(MUX_C3 , (CH & 8));
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

void ReadMPU6050(){
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    angleX = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;
    angleY = atan2(-a.acceleration.x, sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * 180.0 / PI;
}

void TrackSittingTime(){
    if (maxFsrValue > 200) { // Magic num.
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

void CheckTiltAlarmName(){
    if (CurrentState != StateIdle) {
        if (abs(angleX) > 20.0 || abs(angleY) > 20.0) { 
            if (millis() - buzzerMillis >= 300) { 
                buzzerMillis = millis();
                buzzerState = !buzzerState;
                digitalWrite(Buzzer, buzzerState); 
            }
        } 
        else if (CurrentState == StateWarning) {
            if (millis() - buzzerMillis >= 500) { 
                buzzerMillis = millis();
                buzzerState = !buzzerState;
                digitalWrite(Buzzer, buzzerState); 
            }
        } 
        else {
            digitalWrite(Buzzer, LOW); 
        }
    }
}

void allsensor_off(){
    digitalWrite(RelayCh1_Airpump, HIGH); 
    digitalWrite(RelayCh1_Valve, HIGH);    
    digitalWrite(Buzzer, LOW);            
}

void setup() {
    Serial.begin(115200);

    pinMode(Pressure_Sensor, INPUT);
    pinMode(MUX_SIG, INPUT);        
    pinMode(MUX_C0, OUTPUT);
    pinMode(MUX_C1, OUTPUT);
    pinMode(MUX_C2, OUTPUT);
    pinMode(MUX_C3, OUTPUT);    
    pinMode(RelayCh1_Airpump , OUTPUT);
    pinMode(RelayCh1_Valve , OUTPUT);    
    pinMode(Buzzer, OUTPUT);

    Serial.println("Initializing MPU6050...");
    allsensor_off(); 

    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip! Please check your wiring.");
        while (1) {
            delay(10); 
        }
    }
    Serial.println("MPU6050 Found and Initialized successfully!");

    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
}

void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        ReadFSRSensorWithMux();
        ReadPressureSensor();
        ReadMPU6050();
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

        CheckTiltAlarmName();

        switch(CurrentState){
            case StateIdle:
                Serial.println("[STATUS]: IDLE");
                allsensor_off(); 
                break;

            case StateNormal:
                Serial.println("[STATUS]: NORMAL");
                digitalWrite(RelayCh1_Airpump, HIGH);
                digitalWrite(RelayCh1_Valve, HIGH);   
                break;

            case StateWarning:
                Serial.println("[STATUS]: WARNING");
                // Magic number nakub
                if (pressurePsi < 30.0) { 
                    digitalWrite(RelayCh1_Airpump, LOW); 
                } 
                else if (pressurePsi >= 40.0) {
                    digitalWrite(RelayCh1_Airpump, HIGH); 
                }
                digitalWrite(RelayCh1_Valve, HIGH); 
                break;

            case StateDanger:
                Serial.println("[STATUS]: DANGER");
                
                if (dangerStep == 0) {

                    digitalWrite(RelayCh1_Valve, LOW);   
                    digitalWrite(RelayCh1_Airpump, HIGH); 
                    
                    if (millis() - dangerActionMillis >= 5000 || pressurePsi <= 15.0) {
                        dangerStep = 1; 
                        dangerActionMillis = millis(); // 
                    }
                } 
                else if (dangerStep == 1) {
                    digitalWrite(RelayCh1_Valve, HIGH);   
                    digitalWrite(RelayCh1_Airpump, LOW);  
                    
                    if (pressurePsi >= 35.0) {
                        dangerStep = 0; 
                        dangerActionMillis = millis();
                    }
                }
                break;
        }
    }
}