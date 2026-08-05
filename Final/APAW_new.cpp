#include <Wire.h>
//                  INPUT PIN
const int MUX_SIG             = A0;
const int Pressure_Sensor     = A4;
//                  OUTPUT PIN
const int MUX_S0              = 8;
const int MUX_S1              = 9;
const int MUX_S2              = 10;
const int MUX_S3              = 11;

const int RelayCh1_Airpump    = 4;      // Active HIGH
const int RelayCh1_Valve      = 3;      // Active HIGH
const int Buzzer              = 7;      // Active LOW
//                  CONSTANT
const float TargetPressure      = 3.0;      // PSI
const float PressureThreshold   = 32.0;     // mmHg

const float Offset              = 0.479;

const unsigned long FSRAlertTime      = 5UL * 60UL * 1000UL;      // 5 นาที
const unsigned long SittingAlertTime  = 20UL * 60UL * 1000UL;     // 20 นาที

const unsigned long DebugInterval      = 1000;
const unsigned long FirebaseInterval   = 500;
const float PressureLow  = 2.8;   
const float PressureHigh = 3.2;   
//                  SYSTEM STATE
enum SystemState
{
    StateBeforeSitting,
    StateReadyForSit,
    StateMonitoring,
    StatePressureControl,
    StateAlert,
    StateStandUp
};

SystemState CurrentState = StateBeforeSitting;
SystemState LastState    = StateBeforeSitting;

//              SENSOR VARIABLE
int         fsrValues[6];
float       fsrMmHg[6];
int         maxFsrIndex = 0;
float       maxFsrValue = 0;
int         pressureADC = 0;
float       pressureVoltage = 0;
float       pressurePsi = 0;

//              TIMER VARIABLE
unsigned long previousDebugMillis = 0;
unsigned long previousFirebaseMillis = 0;
unsigned long sittingStartMillis = 0;
unsigned long dominantFSRStartMillis = 0;
unsigned long buzzerMillis = 0;

//              USER STATUS
bool userSitting = false;
bool pressureReady = false;
bool pumpStatus = false;
bool valveStatus = false;
bool buzzerStatus = false;

//              DOMINANT FSR
int dominantFSR = -1;
int previousDominantFSR = -1;

//              DISPLAY STRING
String currentStateString = "BEFORE_SITTING";
String currentMessage = "";

void StatePressureControl()
{
    currentStateString = "PRESSURE_CONTROL";
    currentMessage = "Adjusting Pressure";

    digitalWrite(RelayCh1_Valve, LOW);
    valveStatus = false;

    if (!userSitting)
    {
        CurrentState = StateStandUp;
        return;
    }

    if (pressurePsi < PressureHigh)
    {
        digitalWrite(RelayCh1_Airpump, HIGH);
        pumpStatus = true;
    }
    else
    {
        digitalWrite(RelayCh1_Airpump, LOW);
        pumpStatus = false;

        CurrentState = StateMonitoring;
    }
}

//              SELECT MUX CHANNEL
void SelectMUXChannel(uint8_t channel)
{
    digitalWrite(MUX_S0, bitRead(channel,0));
    digitalWrite(MUX_S1, bitRead(channel,1));
    digitalWrite(MUX_S2, bitRead(channel,2));
    digitalWrite(MUX_S3, bitRead(channel,3));

    delayMicroseconds(100);
}

int AverageAnalogRead(byte pin, byte samples = 5)
{
    long sum = 0;

    for(int i=0;i<samples;i++)
    {
        sum += analogRead(pin);
    }
    return sum / samples;
}

void ReadFSR()
{
    maxFsrValue = 0;
    maxFsrIndex = 0;

    for(int i=0;i<6;i++)
    {
        SelectMUXChannel(i);
        delayMicroseconds(300);
        analogRead(MUX_SIG);       // Dummy Read
        delayMicroseconds(300);

        fsrValues[i] = AverageAnalogRead(MUX_SIG);
        fsrMmHg[i] = (fsrValues[i] / 1023.0) * 120.0;

        if(fsrMmHg[i] > maxFsrValue)
        {
            maxFsrValue = fsrMmHg[i];
            maxFsrIndex = i;
        }
    }
}

void ReadPressure()
{
    pressureADC = AverageAnalogRead(Pressure_Sensor);

    pressureVoltage = pressureADC * 5.0 / 1023.0;

    float pressureMPa = (pressureVoltage - Offset) * (1.6 / 4.0);

    if(pressureMPa < 0)
        pressureMPa = 0;

    pressurePsi = pressureMPa * 145.038;
}
void CheckUserSitting()
{
    if(maxFsrValue >= PressureThreshold)
    {
        userSitting = true;
    }
    else
    {
        userSitting = false;
    }
}
void CheckPressureReady()
{
    pressureReady = (pressurePsi >= TargetPressure);
}
void FindDominantFSR()
{
    dominantFSR = maxFsrIndex;
}
void ReadAllSensor()
{
    ReadFSR();
    ReadPressure();
    CheckUserSitting();
    CheckPressureReady();
    FindDominantFSR();
}
void StateBeforeSitting()
{
    currentStateString = "BEFORE_SITTING";

    if(!pressureReady)
    {
        digitalWrite(RelayCh1_Airpump, HIGH);     // Active HIGH
        digitalWrite(RelayCh1_Valve, LOW);

        pumpStatus = true;
        valveStatus = false;

        if(userSitting)
        {
            currentMessage = "Pressure is not ready";

            digitalWrite(Buzzer, LOW);       
            buzzerStatus = true;
        }
        else
        {
            digitalWrite(Buzzer, HIGH);

            buzzerStatus = false;
        }
    }
    else
    {
        digitalWrite(RelayCh1_Airpump, LOW);
        pumpStatus = false;
        digitalWrite(Buzzer, HIGH);
        buzzerStatus = false;
        CurrentState = StateReadyForSit;
    }
}
void StateReadyForSit()
{
    currentStateString = "READY_FOR_SIT";
    currentMessage = "Ready For Sit";

    digitalWrite(RelayCh1_Airpump, LOW);
    digitalWrite(RelayCh1_Valve, LOW);
    digitalWrite(Buzzer, HIGH);

    pumpStatus = false;
    valveStatus = false;
    buzzerStatus = false;

    
    if(userSitting)
    {
        sittingStartMillis = millis();
        dominantFSRStartMillis = millis();
        previousDominantFSR = dominantFSR;
        CurrentState = StateMonitoring;
    }
}
void StateMonitoring()
{
    currentStateString = "MONITORING";
    currentMessage = "Monitoring...";

    if (!userSitting)
    {
        CurrentState = StateStandUp;
        return;
    }

    if (pressurePsi < PressureLow)
    {
        CurrentState = StatePressureControl;
        return;
    }

    if (dominantFSR != previousDominantFSR)
    {
        previousDominantFSR = dominantFSR;
        dominantFSRStartMillis = millis();
    }

    if (millis() - dominantFSRStartMillis >= FSRAlertTime)
    {
        CurrentState = StateAlert;
        return;
    }

    if (millis() - sittingStartMillis >= SittingAlertTime)
    {
        CurrentState = StateAlert;
        return;
    }
}

void StateAlert()
{
    currentStateString = "ALERT";
    currentMessage = "Please Change Sitting Position";

    digitalWrite(Buzzer, LOW);

    buzzerStatus = true;

    if (!userSitting)
    {
        CurrentState = StateStandUp;
        return;
    }

    if (dominantFSR != previousDominantFSR)
    {
        digitalWrite(Buzzer, HIGH);
        buzzerStatus = false;

        previousDominantFSR = dominantFSR;
        dominantFSRStartMillis = millis();
        sittingStartMillis = millis();
        CurrentState = StateMonitoring;
    }
}

void StateStandUp()
{
    currentStateString = "STAND_UP";
    currentMessage = "Stand Up";

    digitalWrite(Buzzer, HIGH);
    digitalWrite(RelayCh1_Airpump, LOW);
    digitalWrite(RelayCh1_Valve, LOW);

    pumpStatus = false;
    valveStatus = false;
    buzzerStatus = false;
    sittingStartMillis = 0;
    dominantFSRStartMillis = 0;
    previousDominantFSR = -1;
    dominantFSR = -1;
    CurrentState = StateBeforeSitting;
}

void PrintDebug()
{
    Serial.println();

    Serial.print("State : ");
    Serial.println(currentStateString);

    Serial.print("Message : ");
    Serial.println(currentMessage);

    Serial.print("Pressure : ");
    Serial.print(pressurePsi);
    Serial.println(" PSI");

    Serial.print("Dominant FSR : ");
    Serial.println(dominantFSR + 1);

    Serial.print("FSR : ");

    for(int i=0;i<6;i++)
    {
        Serial.print(fsrMmHg[i],1);
        Serial.print("  ");
    }

    Serial.println();

    Serial.print("Pump : ");
    Serial.println(pumpStatus);

    Serial.print("Valve : ");
    Serial.println(valveStatus);

    Serial.print("Buzzer : ");
    Serial.println(buzzerStatus);

    Serial.println("----------------------------------------");
}

void setup()
{
    Serial.begin(115200);

    analogReadResolution(10);

    pinMode(MUX_SIG, INPUT);
    pinMode(Pressure_Sensor, INPUT);

    pinMode(MUX_S0, OUTPUT);
    pinMode(MUX_S1, OUTPUT);
    pinMode(MUX_S2, OUTPUT);
    pinMode(MUX_S3, OUTPUT);

    pinMode(RelayCh1_Airpump, OUTPUT);
    pinMode(RelayCh1_Valve, OUTPUT);

    pinMode(Buzzer, OUTPUT);

    digitalWrite(RelayCh1_Airpump, LOW);
    digitalWrite(RelayCh1_Valve, LOW);
    digitalWrite(Buzzer, HIGH);

    CurrentState = StateBeforeSitting;

    Serial.println("APAW System Ready");
}

void loop()
{
    ReadAllSensor();

    switch(CurrentState)
    {
        case StateBeforeSitting:
            StateBeforeSitting();
            break;

        case StateReadyForSit:
            StateReadyForSit();
            break;

        case StateMonitoring:
            StateMonitoring();
            break;

        case StateAlert:
            StateAlert();
            break;

        case StateStandUp:
            StateStandUp();
            break;
        case StatePressureControl:
            StatePressureControl();
        break;
    }

    if(millis() - previousDebugMillis >= DebugInterval)
    {
        previousDebugMillis = millis();

        PrintDebug();
    }

    if(millis() - previousFirebaseMillis >= FirebaseInterval)
    {
        previousFirebaseMillis = millis();

        // UploadDashboard();
    }
}