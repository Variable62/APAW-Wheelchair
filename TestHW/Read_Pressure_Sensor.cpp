const int Pressure_Sensor = A4;

const float OffSet = 0.479;

int pressureADC = 0;
float pressureVoltage = 0;
float pressurePsi = 0;

int AverageAnalogRead(uint8_t pin, uint8_t samples = 50)
{
    long sum = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += analogRead(pin);
    }

    return sum / samples;
}

void ReadPressureSensor()
{
    pressureADC = AverageAnalogRead(Pressure_Sensor);

    pressureVoltage = pressureADC * 5.0 / 16383.0;

    // Datasheet : 0.5V = 0 MPa, 4.5V = 1.6 MPa
    float pressureMPa = (pressureVoltage - OffSet) * (1.6 / 4.0);

    if (pressureMPa < 0)
        pressureMPa = 0;

    pressurePsi = pressureMPa * 145.038;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(14);
}

void loop() {

ReadPressureSensor();

Serial.print("ADC : ");
Serial.print(pressureADC);

Serial.print("   Voltage : ");
Serial.print(pressureVoltage, 3);

Serial.print(" V");

Serial.print("   Pressure : ");
Serial.print(pressurePsi, 2);

Serial.println(" PSI");
}
// ============================================================
//        PRESSURE SENSOR TEST - APAW WHEELCHAIR
//        Arduino UNO R4 WiFi
// ============================================================

// ---------------- Pressure Sensor ----------------
const int Pressure_Sensor = A2;
const float ZeroVoltage = 0.520;

const float SensorMaxMPa = 1.6;
const float SensorMaxPSI = SensorMaxMPa * 145.038;

const float PSI_PER_VOLT = SensorMaxPSI / 4.0;
const float LowVoltageErrorThreshold = 0.020;


// ---------------- Variables ----------------
int pressureADC = 0;

float pressureVoltage = 0.0;
float pressurePsi = 0.0;

int AverageAnalogRead(uint8_t pin, uint8_t samples = 50)
{
    long sum = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        sum += analogRead(pin);
        delayMicroseconds(200);
    }

    return sum / samples;
}

void ReadPressureSensor()
{
    // Read ADC
    pressureADC = AverageAnalogRead(Pressure_Sensor, 50);

    // Convert 14-bit ADC to Voltage
    pressureVoltage =
        pressureADC * 5.0 / 16383.0;

    if (pressureVoltage < (ZeroVoltage - LowVoltageErrorThreshold))
    {
        // Voltage ต่ำกว่า Zero มากเกินไป
        // ไม่ให้ระบบตีความเป็น 0 PSI
        pressurePsi = 0.0;
        return;
    }

    pressurePsi =
        (pressureVoltage - ZeroVoltage) * PSI_PER_VOLT;


    if (pressurePsi < 0)
    {
        pressurePsi = 0;
    }
}

void PrintPressureData()
{
    Serial.print("ADC : ");
    Serial.print(pressureADC);

    Serial.print("   Voltage : ");
    Serial.print(pressureVoltage, 3);

    Serial.print(" V");

    if (pressureVoltage < (ZeroVoltage - LowVoltageErrorThreshold))
    {
        Serial.println("   Pressure : SENSOR/POWER ERROR");

        Serial.print("        Voltage is too low. Expected >= ");
        Serial.print(ZeroVoltage - LowVoltageErrorThreshold, 3);
        Serial.println(" V");

        Serial.println(
            "        Check 5V Supply / GND / Relay / Battery"
        );

        Serial.println(
            "------------------------------------------------------------"
        );

        return;
    }

    Serial.print("   Pressure : ");
    Serial.print(pressurePsi, 2);
    Serial.println(" PSI");

    if (pressurePsi < 0.5)
    {
        Serial.println("        Status : NO / VERY LOW AIR");
    }

    else if (pressurePsi < 2.0)
    {
        Serial.println("        Status : BELOW TARGET");
    }

    else if (pressurePsi <= 2.5)
    {
        Serial.println("        Status : TARGET RANGE");
    }

    else
    {
        Serial.println("        Status : ABOVE TARGET");
    }


    Serial.println(
        "------------------------------------------------------------"
    );
}

void setup()
{
    Serial.begin(115200);

    // UNO R4 WiFi ADC = 14 bit
    analogReadResolution(14);

    pinMode(Pressure_Sensor, INPUT);

    delay(2000);

    Serial.println();
    Serial.println("==============================================");
    Serial.println("      APAW PRESSURE SENSOR TEST");
    Serial.println("==============================================");

    Serial.print("Zero Voltage : ");
    Serial.print(ZeroVoltage, 3);
    Serial.println(" V");

    Serial.print("Sensor Range : 0 - ");
    Serial.print(SensorMaxPSI, 2);
    Serial.println(" PSI");

    Serial.print("PSI / Volt   : ");
    Serial.println(PSI_PER_VOLT, 2);

    Serial.println();
    Serial.println("Cushion Target : 2.0 - 2.5 PSI");
    Serial.println();
}

void loop()
{
    ReadPressureSensor();
    PrintPressureData();

    delay(500);
}