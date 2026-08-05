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