/*
*
Buzzer : active LOW
Relay Active LOW

Upload code 
USB = ถอด
12V = เปิด
รอแปบ
USB = เสียบ
ทำงานได้

Hardware
เริ่ม
เติมลม 2 นาที
-> พร้อมนั่ง (Idle)
-> มีคนนั่ง (Normal)
-> นั่งเกิน 30 นาที (Warning) Buzzer เตือน + เติมลม 2 นาที
-> นั่งนานเกิน 60 นาที (Danger) Buzzer เตือน เปิดวาล + ปิดวาลว์ -> เติมลม 2 นาที loop ต่อไป จนกว่าจะเปลี่ยนลุกเปลี่ยนท่านั่ง
Soft ware app 
-Web App ของญาติเเละผู้ป่วย : ดูข้อมูลของ Wheel chair ตัวเองเเละทราบการเเจ้งเตือนและสถานะค่าต่างๆ (สามารถดูประวัติย้อนหลังได้)*/

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

const unsigned long WarningTime = 30;
const unsigned long DangerTime = 60;
const unsigned long BuzzerInterval = 500;

const unsigned long WarningPumpTime = 120000;
const unsigned long PrePumpTime = 120000;
unsigned long prePumpStartMillis = 0;

const char* currentStateStr = "IDLE";


const char* WIFI_SSID = "USER"; // ใส่ชื่อไวไฟนะ
const char* WIFI_PASSWORD = "PASSWORD"; // รหัส

const char* FIREBASE_HOST =
    "apaw-wheelchair-default-rtdb.asia-southeast1.firebasedatabase.app";

const unsigned long FirebaseInterval = 1000;
//const unsigned long FirebaseInterval = 5000;

unsigned long previousFirebaseMillis = 0;

WiFiSSLClient dashboardClient;
WiFiSSLClient historyClient;


WiFiUDP ntpUDP;

NTPClient timeClient(
    ntpUDP,
    "pool.ntp.org",
    7 * 3600,
    60000
);

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
    Serial.println("Connecting WiFi");

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long startAttempt = millis();

    // รอเชื่อม Wi-Fi
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");

        if (millis() - startAttempt > 20000)
        {
            Serial.println();
            Serial.println("WiFi connection timeout!");
            return;
        }
    }

    Serial.println();
    Serial.println("WiFi connected");

    Serial.print("Waiting for IP");

    unsigned long ipStart = millis();

    while (WiFi.localIP() == IPAddress(0, 0, 0, 0))
    {
        delay(200);
        Serial.print(".");

        if (millis() - ipStart > 10000)
        {
            Serial.println();
            Serial.println("ERROR: No IP address!");
            return;
        }
    }

    Serial.println();

    IPAddress ip = WiFi.localIP();

    Serial.print("IP: ");
    Serial.println(ip);

    Serial.print("Gateway: ");
    Serial.println(WiFi.gatewayIP());

    Serial.print("Subnet: ");
    Serial.println(WiFi.subnetMask());

    Serial.print("RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    Serial.println("WiFi Ready");
}
String GetDate()
{
    time_t rawTime = timeClient.getEpochTime();

    struct tm *timeInfo = localtime(&rawTime);

    char buffer[11];

    sprintf(
        buffer,
        "%02d/%02d/%04d",
        timeInfo->tm_mday,
        timeInfo->tm_mon + 1,
        timeInfo->tm_year + 1900
    );

    return String(buffer);
}

String GetTime()
{
    time_t rawTime = timeClient.getEpochTime();

    struct tm *timeInfo = localtime(&rawTime);

    char buffer[9];

    sprintf(
        buffer,
        "%02d:%02d:%02d",
        timeInfo->tm_hour,
        timeInfo->tm_min,
        timeInfo->tm_sec
    );

    return String(buffer);
}
void UploadDashboard()
{
if (WiFi.status() != WL_CONNECTED)
{
    Serial.println("Firebase: WiFi disconnected");
    return;
}

if (WiFi.localIP() == IPAddress(0, 0, 0, 0))
{
    Serial.println("Firebase: No IP address");
    return;
}

    String json = "{";

    json += "\"state\":\"";
    json += currentStateStr;
    json += "\",";

    json += "\"message\":\"";
    
    if (CurrentState == StateIdle)
    {
        json += "Ready for sit";
    }
    else if (CurrentState == StateNormal)
    {
        json += "Normal";
    }
    else if (CurrentState == StateWarning)
    {
        json += "Please change sitting position";
    }
    else if (CurrentState == StateDanger)
    {
        json += "Danger - Please change sitting position";
    }
    else if (CurrentState == StatePrePump)
    {
        json += "Preparing cushion";
    }
    else
    {
        json += "";
    }

    json += "\",";

    json += "\"fsr1\":";
    json += String(fsrMmHg[0], 1);
    json += ",";

    json += "\"fsr2\":";
    json += String(fsrMmHg[1], 1);
    json += ",";

    json += "\"fsr3\":";
    json += String(fsrMmHg[2], 1);
    json += ",";

    json += "\"fsr4\":";
    json += String(fsrMmHg[3], 1);
    json += ",";

    json += "\"fsr5\":";
    json += String(fsrMmHg[4], 1);
    json += ",";

    json += "\"fsr6\":";
    json += String(fsrMmHg[5], 1);
    json += ",";

    json += "\"pump\":";
    json += pump_status ? "true" : "false";
    json += ",";

    json += "\"valve\":";
    json += valve_status ? "true" : "false";
    json += ",";

    json += "\"sittingMinute\":";
    json += String(sittingDurationMinutes);

    json += "}";

    String path = "/dashboard.json";

    if (dashboardClient.connect(FIREBASE_HOST, 443))
    {
        dashboardClient.println("PUT " + path + " HTTP/1.1");
        dashboardClient.println("Host: " + String(FIREBASE_HOST));
        dashboardClient.println("Content-Type: application/json");
        dashboardClient.print("Content-Length: ");
        dashboardClient.println(json.length());
        dashboardClient.println("Connection: close");
        dashboardClient.println();
        dashboardClient.println(json);

        unsigned long timeout = millis();

        while (dashboardClient.connected() &&
               millis() - timeout < 3000)
        {
            while (dashboardClient.available())
            {
                String line = dashboardClient.readStringUntil('\n');

                if (line.startsWith("HTTP/1.1"))
                {
                    Serial.print("Firebase: ");
                    Serial.println(line);
                }

                timeout = millis();
            }
        }

        dashboardClient.stop();
    }
    else
    {
        Serial.println("Firebase connection failed");
    }
}
void UploadHistory()
{
if (WiFi.status() != WL_CONNECTED)
{
    Serial.println("Firebase: WiFi disconnected");
    return;
}

if (WiFi.localIP() == IPAddress(0, 0, 0, 0))
{
    Serial.println("Firebase: No IP address");
    return;
}

    String json = "{";

    // State
    json += "\"state\":\"";
    json += currentStateStr;
    json += "\",";

    // Message
    json += "\"message\":\"";

    if (CurrentState == StateIdle)
    {
        json += "Ready for sit";
    }
    else if (CurrentState == StateNormal)
    {
        json += "Normal";
    }
    else if (CurrentState == StateWarning)
    {
        json += "Please change sitting position";
    }
    else if (CurrentState == StateDanger)
    {
        json += "Danger - Please change sitting position";
    }
    else if (CurrentState == StatePrePump)
    {
        json += "Preparing cushion";
    }
    else
    {
        json += "";
    }

    json += "\",";

    // FSR
    json += "\"fsr1\":";
    json += String(fsrMmHg[0], 1);
    json += ",";

    json += "\"fsr2\":";
    json += String(fsrMmHg[1], 1);
    json += ",";

    json += "\"fsr3\":";
    json += String(fsrMmHg[2], 1);
    json += ",";

    json += "\"fsr4\":";
    json += String(fsrMmHg[3], 1);
    json += ",";

    json += "\"fsr5\":";
    json += String(fsrMmHg[4], 1);
    json += ",";

    json += "\"fsr6\":";
    json += String(fsrMmHg[5], 1);
    json += ",";

    // Pump
    json += "\"pump\":";
    json += pump_status ? "true" : "false";
    json += ",";

    // Valve
    json += "\"valve\":";
    json += valve_status ? "true" : "false";
    json += ",";

    // Sitting time
    json += "\"sittingMinute\":";
    json += String(sittingDurationMinutes);

    json += "}";

    String recordID = "record_" + String(millis());

    String path = "/history/" + recordID + ".json";

    Serial.println();
    Serial.println("Uploading History...");
    Serial.print("Path: ");
    Serial.println(path);

    if (historyClient.connect(FIREBASE_HOST, 443))
    {
        historyClient.println("PUT " + path + " HTTP/1.1");
        historyClient.println("Host: " + String(FIREBASE_HOST));
        historyClient.println("Content-Type: application/json");

        historyClient.print("Content-Length: ");
        historyClient.println(json.length());

        historyClient.println("Connection: close");
        historyClient.println();

        historyClient.println(json);

        unsigned long timeout = millis();

        while (historyClient.connected() &&
               millis() - timeout < 3000)
        {       
            while (historyClient.available())
            {
                String line =
                    historyClient.readStringUntil('\n');

                if (line.startsWith("HTTP/1.1"))
                {
                    Serial.print("History: ");
                    Serial.println(line);
                }

                timeout = millis();
            }
        }

        historyClient.stop();

        Serial.println("History upload finished.");
    }
    else
    {
        Serial.println("History connection failed");
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

        analogRead(MUX_SIG);

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

        sittingDurationMinutes =
            (millis() - sittingStartTime) / 60000;
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

            if (buzzerState)
            {
                digitalWrite(Buzzer, LOW);
            }
            else
            {
                digitalWrite(Buzzer, HIGH);
            }
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

void UpdateSensorData()
{
    fsr1 = (fsrValues[0] / 1023.0) * 120.0;
    fsr2 = (fsrValues[1] / 1023.0) * 120.0;
    fsr3 = (fsrValues[2] / 1023.0) * 120.0;
    fsr4 = (fsrValues[3] / 1023.0) * 120.0;
    fsr5 = (fsrValues[4] / 1023.0) * 120.0;
    fsr6 = (fsrValues[5] / 1023.0) * 120.0;
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

        if (i < 5)
        {
            Serial.print("\t");
        }
    }

    Serial.println();

    Serial.print("Sitting : ");
    Serial.print(sittingDurationMinutes);
    Serial.print(" min");

    Serial.print(" | Pump : ");
    Serial.print(pump_status ? "ON" : "OFF");

    Serial.print(" | Valve : ");
    Serial.print(valve_status ? "ON" : "OFF");

    Serial.print(" | State : ");
    Serial.println(currentStateStr);

    Serial.println("---------------------------------------------");
}

void setup()
{
    Serial.begin(115200);

    analogReadResolution(10);

    delay(1500);
    ConnectWiFi();
timeClient.begin();
timeClient.update();    
    pinMode(MUX_SIG, INPUT);

    pinMode(MUX_S0, OUTPUT);
    pinMode(MUX_S1, OUTPUT);
    pinMode(MUX_S2, OUTPUT);
    pinMode(MUX_S3, OUTPUT);

    pinMode(RelayCh1_Airpump, OUTPUT);
    pinMode(RelayCh1_Valve, OUTPUT);

    pinMode(Buzzer, OUTPUT);

    AllSensorOff();

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
            else if (maxFsrMmHg >= PressureThreshold &&
                     sittingDurationMinutes >= DangerTime)
            {
                if (CurrentState != StateDanger)
                {
                    CurrentState = StateDanger;

                    dangerStep = 0;

                    dangerActionMillis = millis();
                }
            }
            else if (maxFsrMmHg >= PressureThreshold &&
                     sittingDurationMinutes >= WarningTime)
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
    UpdateSensorData();
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
            if (CurrentState != StatePrePump &&
                LastState != StatePrePump)
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