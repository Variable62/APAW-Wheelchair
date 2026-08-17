#include <Wire.h>

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

const unsigned long WarningTime = 1;
const unsigned long DangerTime = 4;
const unsigned long BuzzerInterval = 500;

const unsigned long WarningPumpTime = 120000;

const char* currentStateStr = "IDLE";

float fsr1 = 0.0;
float fsr2 = 0.0;
float fsr3 = 0.0;
float fsr4 = 0.0;
float fsr5 = 0.0;
float fsr6 = 0.0;

bool pump_status = false;
bool valve_status = false;

enum SystemState {
    StateIdle,
    StateNormal,
    StateWarning,
    StateDanger
};

SystemState CurrentState = StateIdle;
SystemState LastState = StateIdle;

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

    pinMode(MUX_SIG, INPUT);

    pinMode(MUX_S0, OUTPUT);
    pinMode(MUX_S1, OUTPUT);
    pinMode(MUX_S2, OUTPUT);
    pinMode(MUX_S3, OUTPUT);

    pinMode(RelayCh1_Airpump, OUTPUT);
    pinMode(RelayCh1_Valve, OUTPUT);

    pinMode(Buzzer, OUTPUT);

    AllSensorOff();

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

        if (maxFsrMmHg < 70)
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

        CheckStateAlarm();

        if (CurrentState != LastState)
        {
            UpdateSensorData();
            LastState = CurrentState;
        }

        switch (CurrentState)
        {
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

        previousFirebaseMillis = millis();
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