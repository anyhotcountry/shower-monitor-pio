#include <Arduino.h>

#include <math.h>
#include "ESP8266WiFi.h"
#include "PubSubClient.h"
#include "Arduino_JSON.h"
#include "MD_MAXPanel.h"
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "secrets.h"

#define DEBUG 0

#if DEBUG
#define PRINT(s, x)     \
  {                     \
    Serial.print(F(s)); \
    Serial.print(x);    \
  }
#define PRINTS(x) Serial.print(F(x))
#define PRINTD(x) Serial.print(x, DEC)
#else
#define PRINT(s, x)
#define PRINTS(x)
#define PRINTD(x)
#endif

#define MSG_BUFFER_SIZE (512)
unsigned long lastTime = millis();

WiFiClient espClient;
PubSubClient client(espClient);
AsyncWebServer server(80);
char msg[MSG_BUFFER_SIZE];
char textBuffer[10];
unsigned long count = 0;

#define HARDWARE_TYPE MD_MAX72XX::DR0CR0RR1_HW
const uint8_t X_DEVICES = 2;
const uint8_t Y_DEVICES = 1;

#define CLK_PIN D7
#define DATA_PIN D5
#define CS_PIN D6
MD_MAXPanel mp = MD_MAXPanel(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, X_DEVICES, Y_DEVICES);

void printText(char *input)
{
  mp.clear();
  mp.drawText(0, 6, input);
}

void printResults(float level, float litres)
{
  if (level >= 100.0)
  {
    count++;
    mp.clear();
    int row = (int)((count / 16) % 8);
    int col = (int)(count % 16);
    mp.setPoint(col, row);
  }
  else
  {
    sprintf(textBuffer, "%.0f", level);
    printText(textBuffer);
  }
}

void setup_wifi()
{
  printText("Con");
  delay(10);
  // We start by connecting to a WiFi network
  PRINTS("\nConnecting to:");
  PRINT(" ", ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    PRINTS(".");
  }

  randomSeed(micros());

  PRINTS("\nWiFi connected");
  PRINT("\nIP address: ", WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hi! I am ESP8266."); });

  AsyncElegantOTA.begin(&server); // Start ElegantOTA
  server.begin();
  PRINTS("\nHTTP server started");
}

void callback(char *topic, byte *payload, unsigned int length)
{
  memcpy(msg, payload, length);
  // Append NULL terminator
  msg[length] = '\0';
  PRINT("\nMessage arrived: ", msg);
  JSONVar myObject = JSON.parse(msg);
  float level = (float)(double)myObject["level"];
  float litres = (float)(double)myObject["litres"];
  printResults(level, litres);
  lastTime = millis();
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    PRINTS("\nAttempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str()))
    {
      PRINTS("\nConnected.");
      client.subscribe("home/shower");
    }
    else
    {
      printText("CEr");
      PRINT("\nFailed, rc=", client.state());
      PRINTS("\nTry again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  mp.begin();
  mp.setIntensity(1);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  if (millis() - lastTime > 10000)
  {
    printText("Err");
    delay(1000);
  }
}
