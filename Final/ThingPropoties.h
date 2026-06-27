#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <Arduino_NetworkConfigurator.h>
#include "configuratorAgents/agents/BLEAgent.h"
#include "configuratorAgents/agents/SerialAgent.h"

String state;
int fsr1;
int fsr2;
int fsr3;
int fsr4;
int fsr5;
int fsr6;
int GyX;
int GyY;
bool led_danger;
bool led_normal;
bool led_warning;
bool pump_status;
bool valve_status;

KVStore kvStore;
BLEAgentClass BLEAgent;
SerialAgentClass SerialAgent;
WiFiConnectionHandler ArduinoIoTPreferredConnection; 
NetworkConfiguratorClass NetworkConfigurator(ArduinoIoTPreferredConnection);

void initProperties(){
  NetworkConfigurator.addAgent(BLEAgent);
  NetworkConfigurator.addAgent(SerialAgent);
  NetworkConfigurator.setStorage(kvStore);
  // For changing the default reset pin uncomment and set your preferred pin. Use DISABLE_PIN for disabling the reset procedure.
  //NetworkConfigurator.setReconfigurePin(your_pin);
  ArduinoCloud.setConfigurator(NetworkConfigurator);

  ArduinoCloud.addProperty(state, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(fsr1, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(fsr2, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(fsr3, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(fsr4, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(fsr5, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(fsr6, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(GyX, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(GyY, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(led_danger, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(led_normal, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(led_warning, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(pump_status, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(valve_status, READ, ON_CHANGE, NULL);

}
