#include "thingProperties.h"

unsigned long previousMillis = 0;
const long interval = 1000; 

void setup() {

  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }

  initProperties();

  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

void loop() {

  ArduinoCloud.update();
  
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    
    // Example condition
    bool systemHealthy = true; 
    bool systemWarning = false;
    bool systemError = false;

    if (systemHealthy) {
      led_green = true;   
      led_yellow = false; 
      led_red = false;    
    }
  }
}

void onButton1Change()  {
  Serial.print("UI Button 1 changed to: ");
  Serial.println(button_1);

  if (button_1 == true) {
    Serial.println("Action: Button 1 is ON -> Doing task A");

  } else {
    Serial.println("Action: Button 1 is OFF -> Stopping task A");
  }
}

void onButton2Change()  {
  Serial.print("UI Button 2 changed to: ");
  Serial.println(button_2);

  if (button_2 == true) {
    Serial.println("Action: Button 2 is ON -> Doing task B");

  } else {
    Serial.println("Action: Button 2 is OFF -> Stopping task B");
  }
}
