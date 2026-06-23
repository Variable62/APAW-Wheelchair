#include "thingProperties.h"
#include <ArduinoHttpClient.h>
#include <WiFi.h>

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

const String GOOGLE_SCRIPT_ID = "YOUR_SCRIPT_ID_HERE"; 
const char* googleServer      = "script.google.com";
const int googlePort          = 443;

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
SystemState LastState    = StateIdle; 

WiFiSSLClient wifiSSL;
HttpClient httpClient = HttpClient(wifiSSL, googleServer, googlePort);

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
    if (CurrentState == StateWarning) {
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

void sendDataToGoogleSheet() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[Google Sheets] WiFi Disconnected. Cannot send data.");
        return;
    }

    Serial.println("[Google Sheets] Sending data on State Change...");

    String p_stat = (digitalRead(RelayCh1_Airpump) == LOW) ? "ON" : "OFF";
    String v_stat = (digitalRead(RelayCh1_Valve) == LOW) ? "ON" : "OFF";
    String l_norm = led_normal ? "ON" : "OFF";
    String l_warn = led_warning ? "ON" : "OFF";
    String l_dang = led_danger ? "ON" : "OFF";

    String jsonPayload = "{";
    jsonPayload += "\"state\":\"" + state + "\",";
    jsonPayload += "\"fsr1\":" + String(fsrValues[0]) + ",";
    jsonPayload += "\"fsr2\":" + String(fsrValues[1]) + ",";
    jsonPayload += "\"fsr3\":" + String(fsrValues[2]) + ",";
    jsonPayload += "\"fsr4\":" + String(fsrValues[3]) + ",";
    jsonPayload += "\"fsr5\":" + String(fsrValues[4]) + ",";
    jsonPayload += "\"fsr6\":" + String(fsrValues[5]) + ",";
    jsonPayload += "\"pump_status\":\"" + p_stat + "\",";
    jsonPayload += "\"valve_status\":\"" + v_stat + "\",";
    jsonPayload += "\"led_normal\":\"" + l_norm + "\",";
    jsonPayload += "\"led_warning\":\"" + l_warn + "\",";
    jsonPayload += "\"led_danger\":\"" + l_dang + "\"";
    jsonPayload += "}";

    String url = "/macros/s/" + GOOGLE_SCRIPT_ID + "/exec";
    
    httpClient.beginRequest();
    httpClient.post(url, "application/json", jsonPayload);
    httpClient.endRequest();

    int statusCode = httpClient.responseStatusCode();
    String response = httpClient.responseBody();

    Serial.print("[Google Sheets] HTTP Status Code: ");
    Serial.println(statusCode);
    Serial.print("[Google Sheets] Response: ");
    Serial.println(response);
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
    
    Serial.print(" || PUMP: "); Serial.print(digitalRead(RelayCh1_Airpump) == LOW ? "ON" : "OFF");
    Serial.print(" | VALVE: "); Serial.print(digitalRead(RelayCh1_Valve) == LOW ? "ON" : "OFF");
    
    Serial.print(" || SYSTEM STATE: ");
    Serial.println(state);
    Serial.println("------------------------------------------------------------------------------------------------------------------------");
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
    httpClient.setTimeout(10000);
}

void loop() {
    ArduinoCloud.update();
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        ReadFSRSensorWithMux();
        ReadPressureSensor();
        TrackSittingTime();

        fsr1 = fsrValues[0];
        fsr2 = fsrValues[1];
        fsr3 = fsrValues[2];
        fsr4 = fsrValues[3];
        fsr5 = fsrValues[4];
        fsr6 = fsrValues[5];

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
        printDebugInfo();

        if (CurrentState != LastState) {
            LastState = CurrentState;
            sendDataToGoogleSheet();
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
                if (pressurePsi < 1.0) { 
                    digitalWrite(RelayCh1_Airpump, LOW); 
                    pump_status = true;
                } 
                else if (pressurePsi >= 1.4) {
                    digitalWrite(RelayCh1_Airpump, HIGH); 
                    pump_status = false;
                }
                digitalWrite(RelayCh1_Valve, HIGH); 
                valve_status = false;
                break;

            case StateDanger:
                if (dangerStep == 0) {
                    digitalWrite(RelayCh1_Valve, LOW);   
                    digitalWrite(RelayCh1_Airpump, HIGH); 
                    valve_status = true;
                    pump_status = false;
                    if (millis() - dangerActionMillis >= 5000 || pressurePsi <= 0.5) {
                        dangerStep = 1; 
                        dangerActionMillis = millis(); 
                    }
                } 
                else if (dangerStep == 1) {
                    digitalWrite(RelayCh1_Valve, HIGH);   
                    digitalWrite(RelayCh1_Airpump, LOW);  
                    valve_status = false;
                    pump_status = true;
                    if (pressurePsi >= 1.3) {
                        dangerStep = 0; 
                        dangerActionMillis = millis();
                    }
                }
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
