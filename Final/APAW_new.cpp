#include <Wire.h>
#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

const int MUX_SIG            = A0;
const int MUX_S0             = 8;
const int MUX_S1             = 9;
const int MUX_S2             = 10;
const int MUX_S3             = 11;

const int RelayCh1_Airpump   = 3;
const int RelayCh1_Valve     = 4;
const int Buzzer             = 5;

int fsrValues[6] = {0, 0, 0, 0, 0, 0};
float fsrMmHg[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

int maxFsrValue = 0;
float maxFsrMmHg = 0.0;

unsigned long sittingStartTime = 0;
unsigned long sittingDurationMinutes = 0;
bool Sitting = false;

unsigned long previousMillis = 0;
const unsigned long interval = 1000;

unsigned long buzzerMillis = 0;
bool buzzerState = false;

unsigned long dangerActionMillis = 0;
unsigned long warningPumpStartMillis = 0;

int dangerStep = 0;

const float PressureThreshold = 32.0;

const unsigned long WarningTime = 30; // 
const unsigned long DangerTime = 60;  // 
const unsigned long BuzzerInterval = 500;

const unsigned long WarningPumpTime = 180000;  //  120000  2 นาที   
const unsigned long PrePumpTime = 180000;

unsigned long prePumpStartMillis = 0;

const char* currentStateStr = "IDLE";

const char* WIFI_SSID = "4G-MIFI-9194";
const char* WIFI_PASSWORD = "1234567890";

const char* FIREBASE_HOST = "apaw-wheelchair-default-rtdb.asia-southeast1.firebasedatabase.app";

const unsigned long FirebaseInterval = 10000;
unsigned long previousFirebaseMillis = 0;

WiFiSSLClient dashboardClient;
WiFiSSLClient historyClient;
WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000);

float fsr1 = 0.0;
float fsr2 = 0.0;
float fsr3 = 0.0;
float fsr4 = 0.0;
float fsr5 = 0.0;
float fsr6 = 0.0;

bool pump_status = false;
bool valve_status = false;

enum SystemState {
    StatePrePump,
    StateIdle,
    StateNormal,
    StateWarning,
    StateDanger
};

SystemState CurrentState = StatePrePump;
SystemState LastState = StatePrePump;

void ConnectWiFi()
{
    Serial.print("Connecting WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startMillis = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        if (millis() - startMillis >= 20000)
        {
            Serial.println();
            Serial.println("WiFi connection timeout");
            return;
        }
    }

    Serial.println();
    Serial.println("WiFi connected");

    Serial.print("Getting IP Address");
    while (WiFi.localIP().toString() == "0.0.0.0")
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

String GetDate()
{
    time_t rawTime = timeClient.getEpochTime();
    struct tm *timeInfo = localtime(&rawTime);
    char buffer[11];
    sprintf(buffer, "%02d/%02d/%04d", timeInfo->tm_mday, timeInfo->tm_mon + 1, timeInfo->tm_year + 1900);
    return String(buffer);
}

String GetTime()
{
    time_t rawTime = timeClient.getEpochTime();
    struct tm *timeInfo = localtime(&rawTime);
    char buffer[9];
    sprintf(buffer, "%02d:%02d:%02d", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
    return String(buffer);
}

void UploadDashboard()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Firebase: WiFi disconnected");
        return;
    }

    String json = "{";
    json += "\"state\":\"" + String(currentStateStr) + "\",";
    json += "\"message\":\"";
    
    if (CurrentState == StateIdle) json += "Ready for sit";
    else if (CurrentState == StateNormal) json += "Normal";
    else if (CurrentState == StateWarning) json += "Please change sitting position";
    else if (CurrentState == StateDanger) json += "Danger - Please change sitting position";
    else if (CurrentState == StatePrePump) json += "Preparing cushion";
    
    json += "\",";
    json += "\"fsr1\":" + String(fsrMmHg[0], 1) + ",";
    json += "\"fsr2\":" + String(fsrMmHg[1], 1) + ",";
    json += "\"fsr3\":" + String(fsrMmHg[2], 1) + ",";
    json += "\"fsr4\":" + String(fsrMmHg[3], 1) + ",";
    json += "\"fsr5\":" + String(fsrMmHg[4], 1) + ",";
    json += "\"fsr6\":" + String(fsrMmHg[5], 1) + ",";
    json += "\"pump\":" + String(pump_status ? "true" : "false") + ",";
    json += "\"valve\":" + String(valve_status ? "true" : "false") + ",";
    json += "\"sittingMinute\":" + String(sittingDurationMinutes);
    json += "}";

    String path = "/dashboard.json";

    Serial.println("[Firebase] Connecting to Dashboard...");

    if (dashboardClient.connect(FIREBASE_HOST, 443))
    {
        Serial.println("[Firebase] Connected! Sending Dashboard Data...");
        dashboardClient.println("PUT " + path + " HTTP/1.1");
        dashboardClient.println("Host: " + String(FIREBASE_HOST));
        dashboardClient.println("Content-Type: application/json");
        dashboardClient.print("Content-Length: ");
        dashboardClient.println(json.length());
        dashboardClient.println("Connection: close");
        dashboardClient.println();
        dashboardClient.println(json);

        unsigned long timeout = millis();
        while (dashboardClient.connected() && millis() - timeout < 3000)
        {
            while (dashboardClient.available())
            {
                char c = dashboardClient.read();
                Serial.write(c);
                timeout = millis();
            }
        }
        dashboardClient.stop();
        Serial.println("\n[Firebase] Dashboard upload finished.");
    }
    else 
    {
        Serial.println("ERROR: Connection to Firebase failed (Dashboard)!");
    }
}

void UploadHistory()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("History: WiFi disconnected");
        return;
    }

    String json = "{";
    json += "\"date\":\"" + GetDate() + "\",";
    json += "\"time\":\"" + GetTime() + "\",";

    json += "\"state\":\"" + String(currentStateStr) + "\",";
    json += "\"message\":\"";
    
    if (CurrentState == StateIdle) json += "Ready for sit";
    else if (CurrentState == StateNormal) json += "Normal";
    else if (CurrentState == StateWarning) json += "Please change sitting position";
    else if (CurrentState == StateDanger) json += "Danger - Please change sitting position";
    else if (CurrentState == StatePrePump) json += "Preparing cushion";
    
    json += "\",";
    json += "\"fsr1\":" + String(fsrMmHg[0], 1) + ",";
    json += "\"fsr2\":" + String(fsrMmHg[1], 1) + ",";
    json += "\"fsr3\":" + String(fsrMmHg[2], 1) + ",";
    json += "\"fsr4\":" + String(fsrMmHg[3], 1) + ",";
    json += "\"fsr5\":" + String(fsrMmHg[4], 1) + ",";
    json += "\"fsr6\":" + String(fsrMmHg[5], 1) + ",";
    json += "\"pump\":" + String(pump_status ? "true" : "false") + ",";
    json += "\"valve\":" + String(valve_status ? "true" : "false") + ",";
    json += "\"sittingMinute\":" + String(sittingDurationMinutes);
    json += "}";

    String recordID = "record_" + String(millis());
    String path = "/history/" + recordID + ".json";

    Serial.println("[Firebase] Connecting to History...");

    if (historyClient.connect(FIREBASE_HOST, 443))
    {
        Serial.println("[Firebase] Connected! Sending History Data...");
        historyClient.println("PUT " + path + " HTTP/1.1");
        historyClient.println("Host: " + String(FIREBASE_HOST));
        historyClient.println("Content-Type: application/json");
        historyClient.print("Content-Length: ");
        historyClient.println(json.length());
        historyClient.println("Connection: close");
        historyClient.println();
        historyClient.println(json);

        unsigned long timeout = millis();
        while (historyClient.connected() && millis() - timeout < 3000)
        {       
            while (historyClient.available())
            {
                char c = historyClient.read();
                Serial.write(c);
                timeout = millis();
            }
        }
        historyClient.stop();
        Serial.println("\n[Firebase] History upload finished.");
    }
    else
    {
        Serial.println("ERROR: Connection to Firebase failed (History)!");
    }
}

void PrePump()
{
    currentStateStr = "PREPUMP";
    digitalWrite(RelayCh1_Airpump, LOW);
    digitalWrite(RelayCh1_Valve, HIGH);
    digitalWrite(Buzzer, HIGH);

    pump_status = true;
    valve_status = false;

    if (millis() - prePumpStartMillis >= PrePumpTime)
    {
        digitalWrite(RelayCh1_Airpump, HIGH);
        pump_status = false;
        CurrentState = StateIdle;
    }
}

void SelectMUXCh(uint8_t ch)
{
    digitalWrite(MUX_S0, bitRead(ch, 0));
    digitalWrite(MUX_S1, bitRead(ch, 1));
    digitalWrite(MUX_S2, bitRead(ch, 2));
    digitalWrite(MUX_S3, bitRead(ch, 3));
    delayMicroseconds(100);
}

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
        fsrValues[i] = AverageAnalogRead(MUX_SIG);
        fsrMmHg[i] = (fsrValues[i] / 1023.0) * 120.0;

        if (fsrValues[i] > maxFsrValue)
        {
            maxFsrValue = fsrValues[i];
        }
    }
    maxFsrMmHg = (maxFsrValue / 1023.0) * 120.0;
}

void TrackSittingTime()
{
    if (maxFsrMmHg >= PressureThreshold)
    {
        if (!Sitting)
        {
            sittingStartTime = millis();
            Sitting = true;
        }
        sittingDurationMinutes = (millis() - sittingStartTime) / 60000;
    }
    else
    {
        Sitting = false;
        sittingDurationMinutes = 0;
    }
}

void CheckStateAlarm()
{
    if (CurrentState == StateWarning)
    {
        currentStateStr = "WARNING";
        if (millis() - buzzerMillis >= BuzzerInterval)
        {
            buzzerMillis = millis();
            buzzerState = !buzzerState;
            digitalWrite(Buzzer, buzzerState ? LOW : HIGH);
        }
    }
    else if (CurrentState == StateDanger)
    {
        currentStateStr = "DANGER";
        digitalWrite(Buzzer, LOW);
    }
    else if (CurrentState == StateNormal)
    {
        currentStateStr = "NORMAL";
        digitalWrite(Buzzer, HIGH);
    }
    else
    {
        currentStateStr = "IDLE";
        digitalWrite(Buzzer, HIGH);
    }
}

void AllSensorOff()
{
    digitalWrite(RelayCh1_Airpump, HIGH);
    digitalWrite(RelayCh1_Valve, HIGH);
    digitalWrite(Buzzer, HIGH);
}

void printDebugInfo()
{
    Serial.print("[mmHg] ");
    for (int i = 0; i < 6; i++)
    {
        Serial.print("FSR" + String(i + 1) + ":" + String(fsrMmHg[i], 1) + "\t");
    }
    Serial.println();
    Serial.print("Sitting : " + String(sittingDurationMinutes) + " min");
    Serial.print(" | Pump : " + String(pump_status ? "ON" : "OFF"));
    Serial.print(" | Valve : " + String(valve_status ? "ON" : "OFF"));
    Serial.println(" | State : " + String(currentStateStr));
    Serial.println("---------------------------------------------");
}

void setup()
{
    Serial.begin(115200);
    delay(1000);

    analogReadResolution(10);

    pinMode(MUX_SIG, INPUT);
    pinMode(MUX_S0, OUTPUT);
    pinMode(MUX_S1, OUTPUT);
    pinMode(MUX_S2, OUTPUT);
    pinMode(MUX_S3, OUTPUT);

    pinMode(RelayCh1_Airpump, OUTPUT);
    pinMode(RelayCh1_Valve, OUTPUT);
    pinMode(Buzzer, OUTPUT);

    AllSensorOff();

    ConnectWiFi();
    
    timeClient.begin();
    
    // บังคับซิงค์เวลา NTP ให้สำเร็จก่อนเริ่มทำงาน
    Serial.print("Updating NTP time");
    while (!timeClient.update()) {
        timeClient.forceUpdate();
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    Serial.print("NTP Time synced: ");
    Serial.println(GetTime());

    prePumpStartMillis = millis();
    Serial.println("System Ready");
}

void loop()
{
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        ReadFSRSensorWithMux();
        TrackSittingTime();

        if (CurrentState != StatePrePump)
        {
            if (maxFsrMmHg < PressureThreshold)
            {
                CurrentState = StateIdle;
            }
            else if (maxFsrMmHg >= PressureThreshold && sittingDurationMinutes >= DangerTime)
            {
                if (CurrentState != StateDanger)
                {
                    CurrentState = StateDanger;
                    dangerStep = 0;
                    dangerActionMillis = millis();
                }
            }
            else if (maxFsrMmHg >= PressureThreshold && sittingDurationMinutes >= WarningTime)
            {
                if (CurrentState != StateWarning)
                {
                    CurrentState = StateWarning;
                    warningPumpStartMillis = millis();
                }
            }
            else
            {
                CurrentState = StateNormal;
            }
        }

        CheckStateAlarm();
    
        bool stateChanged = false;
        if (CurrentState != LastState)
        {
            stateChanged = true;
        }

        switch (CurrentState)
        {
            case StatePrePump:
                PrePump();
                break;

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
                digitalWrite(RelayCh1_Valve, HIGH);
                valve_status = false;

                if (millis() - warningPumpStartMillis < WarningPumpTime)
                {
                    digitalWrite(RelayCh1_Airpump, LOW);
                    pump_status = true;
                }
                else
                {
                    digitalWrite(RelayCh1_Airpump, HIGH);
                    pump_status = false;
                }
                break;

            case StateDanger:
                if (dangerStep == 0)
                {
                    digitalWrite(RelayCh1_Valve, LOW);
                    digitalWrite(RelayCh1_Airpump, HIGH);
                    valve_status = true;
                    pump_status = false;

                    if (millis() - dangerActionMillis >= 5000)
                    {
                        dangerStep = 1;
                        dangerActionMillis = millis();
                    }
                }
                else if (dangerStep == 1)
                {
                    digitalWrite(RelayCh1_Valve, HIGH);
                    valve_status = false;

                    if (millis() - dangerActionMillis < WarningPumpTime)
                    {
                        digitalWrite(RelayCh1_Airpump, LOW);
                        pump_status = true;
                    }
                    else
                    {
                        digitalWrite(RelayCh1_Airpump, HIGH);
                        pump_status = false;
                        dangerStep = 0;
                        dangerActionMillis = millis();
                    }
                }
                break;
        }

        if (stateChanged)
        {
            if (CurrentState != StatePrePump && LastState != StatePrePump)
            {
                UploadHistory();
            }
            LastState = CurrentState;
        }

        printDebugInfo();

        if (millis() - previousFirebaseMillis >= FirebaseInterval)
        {
            previousFirebaseMillis = millis();
            UploadDashboard();
        }
    }
}