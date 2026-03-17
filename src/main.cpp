#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include <timerCallback.h>
//#include <displayOled.hpp>

Preferences prefs;

const char *ssid = "ESP32-FRIDGE";
const char *password = "12345678";

const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);

//DisplayOled displayOled(5, 6);

const int ONE_WIRE_BUS = 10;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const int LED_PIN = 4;
int TEMP_OFF = -15;
int TEMP_INTERVAL = 2;
int TEMP_ON = TEMP_OFF + TEMP_INTERVAL;
unsigned long TIME_UPDATE = 5 * 60 * 1000;
unsigned long maxCompresorRuntime = 30 * 60 * 1000;
unsigned maxRuntime = maxCompresorRuntime / TIME_UPDATE, currentRuntime = 0;
int currentTemp;

bool compressorActive = false;

String tempHistory[10];
int historyIndex = 0;
int historyCount = 0; 

void addTempToHistory(int temp) 
{
    String newRecord = String(historyCount + 1) + ") " + String(temp);
    
    if (historyCount >= 10) 
    {
      for (int i = 0; i < 9; i++) 
      {
        tempHistory[i] = tempHistory[i + 1];
        int spacePos = tempHistory[i].indexOf(')');
        if (spacePos > 0) 
        {
          tempHistory[i] = String(i + 1) + tempHistory[i].substring(spacePos);
        }
      }
      tempHistory[9] = newRecord;
    } 
    else 
    {
      tempHistory[historyCount] = newRecord;
      historyCount++;
    }
}

int getTempSensor()
{
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  currentTemp = tempC;
  addTempToHistory(currentTemp);
  return tempC;
}

void updateFridge()
{
  int temp = getTempSensor();
  Serial.print("temp= ");
  Serial.println(temp);

  if(compressorActive)
  {
    currentRuntime++;
    if(currentRuntime >= maxRuntime)
    {
      currentRuntime = 0;
      compressorActive = false;
      return;
    }
  }
  
  if (compressorActive == false)
  {
    // warm
    if (temp >= TEMP_ON)
    {
      compressorActive = true;
    }
  }
  else // fridge on
  {
    if (temp <= TEMP_OFF)
    {
      // cool down
      currentRuntime = 0;
      compressorActive = false;
    }
  }

  digitalWrite(LED_PIN, compressorActive ? LOW : HIGH);
}

// void printOled()
// {
//   displayOled.print(ssid, password, currentTemp);
// }

void handleRoot()
{
  if (!LittleFS.exists("/index.html"))
  {
    Serial.println("index.html not found!");
    server.send(500, "text/plain", "index.html not found");
    return;
  }

  File file = LittleFS.open("/index.html", "r");
  if (!file)
  {
    Serial.println("Failed to open index.html");
    server.send(500, "text/plain", "Error opening index.html");
    return;
  }

  Serial.println("Sending index.html, size: " + String(file.size()));
  server.streamFile(file, "text/html");
  file.close();
  Serial.println("File sent successfully");
}

void handleRedirect()
{
  Serial.println("Перенаправление запроса на главную страницу: " + server.uri());
  server.sendHeader("Location", "http://192.168.4.1/", true);
  server.send(302, "text/plain", ""); // 302 - временный редирект
}

void handle204()
{
  Serial.println("Samsung/Android check - returning 204");
  server.send(204, "text/plain", ""); // Пустой ответ с кодом 204
}

void dnsAdressSetup()
{
  server.on("/hotspot-detect.html", handleRedirect);
  server.on("/library/test/success.html", handleRedirect);
  server.on("/success.html", handleRedirect);

  // Windows
  server.on("/ncsi.txt", handleRedirect);
  server.on("/connecttest.txt", handleRedirect);

  // Android
  server.on("/generate_204", handleRedirect);
  server.on("/gen_204", handleRedirect);

  // Другие популярные
  server.on("/fwlink/", handleRedirect);
  server.on("/canonical.html", handleRedirect);

  server.on("/generate_204", handle204);        // Для Android
  server.on("/gen_204", handle204);             // Альтернативный
  server.on("/connectivitycheck.gstatic.com", handle204); // Для новых версий
}

void serveCSS()
{
  File file = LittleFS.open("/css/style.css", "r");
  if (!file)
  {
    server.send(404, "text/plain", "Not found");
    return;
  }
  server.streamFile(file, "text/css");
  file.close();
}

void serveJS()
{
  File file = LittleFS.open("/js/script.js", "r");
  if (!file)
  {
    server.send(404, "text/plain", "Not found");
    return;
  }
  server.streamFile(file, "application/javascript");
  file.close();
}

void handleApi()
{
  JsonDocument doc;
  doc["currentTemp"] = currentTemp;
  doc["tempOff"] = TEMP_OFF;
  doc["tempInterval"] = TEMP_INTERVAL;

  JsonArray history = doc["history"].to<JsonArray>();
  for (int i = 0; i < historyCount; i++) 
  {
    history.add(tempHistory[i]);
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleSendText1()
{
  if (server.hasArg("value1"))
  {
    String value1 = server.arg("value1");
    TEMP_OFF = value1.toInt();
    TEMP_ON = TEMP_OFF + TEMP_INTERVAL;
    prefs.putInt("temp_off", TEMP_OFF);

    Serial.print("Temperature OFF set to: ");
    Serial.print(TEMP_OFF);
    Serial.println("°C");

    server.send(200, "text/plain", "OK");
  }
  else
  {
    server.send(400, "text/plain", "Value not received");
  }
}

void handleSendText2()
{
  if (server.hasArg("value2"))
  {
    String value2 = server.arg("value2");
    TEMP_INTERVAL = value2.toInt();
    TEMP_ON = TEMP_OFF + TEMP_INTERVAL;
    prefs.putInt("temp_interval", TEMP_INTERVAL);

    Serial.print("Update interval set to: ");
    Serial.print(TEMP_INTERVAL);
    Serial.println("C");

    server.send(200, "text/plain", "OK");
  }
  else
  {
    server.send(400, "text/plain", "Value not received");
  }
}

void updateWeb()
{
  server.handleClient();
  dnsServer.processNextRequest();
}

timerCallback ledTimer(updateFridge, TIME_UPDATE);
//timerCallback oledTimer(printOled, 1000);
timerCallback webTimer(updateWeb, 10);

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  prefs.begin("settings", false);
  TEMP_OFF = prefs.getInt("temp_off", -15);
  TEMP_INTERVAL = prefs.getInt("temp_interval", 2);
  TEMP_ON = TEMP_OFF + TEMP_INTERVAL;

  //displayOled.begin();

  getTempSensor();


  if (!LittleFS.begin()) 
  {
    while (1)
    {
      Serial.println("LittleFS mount failed");
      delay(1000);
    }
  }

  WiFi.softAP(ssid, password);
  IPAddress apIP = WiFi.softAPIP();

  dnsServer.start(DNS_PORT, "*", apIP);
  dnsAdressSetup();

  // http://192.168.4.1/
  server.on("/css/style.css", serveCSS);
  server.on("/js/script.js", serveJS);

  server.on("/", handleRoot);
  server.on("/api", handleApi);
  server.on("/sendText", HTTP_GET, [](){
    if (server.hasArg("value1")) handleSendText1();
    if (server.hasArg("value2")) handleSendText2(); });

  server.begin();
}

void loop()
{
  ledTimer.loop();
  //oledTimer.loop();
  webTimer.loop();
}