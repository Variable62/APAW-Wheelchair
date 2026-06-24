#include <WiFiS3.h>
#include <ArduinoHttpClient.h>

// ===== Google Apps Script =====
const char serverAddress[] = "script.google.com";

const char scriptPath[] =
"/macros/s/AKfycbxZjDMASED2fb-KShF5fpRH9hsLarRWIGLeJpabnKcbui6nhsq_TFB22f4CnRL2gwSH/exec";

// ===== SSL Client =====
WiFiSSLClient sslClient;
HttpClient httpClient(sslClient, serverAddress, 443);

// ======================================================
// Send Test Data
// ======================================================

void sendLog()
{
  String json = "{";

  json += "\"state\":\"TEST\",";
  json += "\"pressure\":1.23,";

  json += "\"fsr1\":100,";
  json += "\"fsr2\":200,";
  json += "\"fsr3\":300,";
  json += "\"fsr4\":400,";
  json += "\"fsr5\":500,";
  json += "\"fsr6\":600,";

  json += "\"pump\":\"OFF\",";
  json += "\"valve\":\"ON\",";

  json += "\"gyx\":5.5,";
  json += "\"gyy\":10.2";

  json += "}";

  Serial.println();
  Serial.println("====================================");
  Serial.println("Sending JSON");
  Serial.println(json);
  Serial.println("====================================");

  httpClient.beginRequest();

  httpClient.post(scriptPath);

  httpClient.sendHeader(
      "Content-Type",
      "application/json");

  httpClient.sendHeader(
      "Content-Length",
      json.length());

  httpClient.beginBody();

  httpClient.print(json);

  httpClient.endRequest();

  int statusCode =
      httpClient.responseStatusCode();

  String response =
      httpClient.responseBody();

  Serial.print("HTTP Status : ");
  Serial.println(statusCode);

  Serial.print("Response : ");
  Serial.println(response);
}

// ======================================================

void setup()
{
  Serial.begin(115200);

  while (!Serial);

  delay(5000);

  Serial.println();
  Serial.println("===== HTTPS POST TEST =====");

  sendLog();
}

void loop()
{
}
