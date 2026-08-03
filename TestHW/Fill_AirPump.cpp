const int Pressure_Sensor  = A2;
const int RelayCh1_Airpump = 3;
const int RelayCh1_Valve   = 4;

// Calibration
const int adcMin = 102;
const int adcMax = 921;

const float PressureMinPsi = 0.0;
const float PressureMaxPsi = 5.0;

// Target
const float TargetPressure = 3.0;

float pressurePsi = 0.0;

void setup()
{
    Serial.begin(115200);

    pinMode(Pressure_Sensor, INPUT);
    pinMode(RelayCh1_Airpump, OUTPUT);
    pinMode(RelayCh1_Valve, OUTPUT);

    digitalWrite(RelayCh1_Airpump, HIGH);
    digitalWrite(RelayCh1_Valve, HIGH);

    Serial.println("===== Air Pump Test =====");
}

void loop()
{
    int raw = analogRead(Pressure_Sensor);

    raw = constrain(raw, adcMin, adcMax);

    pressurePsi =
        PressureMinPsi +
        ((float)(raw - adcMin) /
        (adcMax - adcMin)) *
        (PressureMaxPsi - PressureMinPsi);

    Serial.print("RAW = ");
    Serial.print(raw);

    Serial.print("   PSI = ");
    Serial.print(pressurePsi, 2);

    if (pressurePsi < TargetPressure)
    {
        digitalWrite(RelayCh1_Airpump, LOW);   // Pump ON
        digitalWrite(RelayCh1_Valve, HIGH);    // Valve OFF

        Serial.println("   --> Pump ON");
    }
    else
    {
        digitalWrite(RelayCh1_Airpump, HIGH);  // Pump OFF
        digitalWrite(RelayCh1_Valve, HIGH);    // Valve OFF

        Serial.println("   --> Pump OFF");
    }

    delay(200);
}