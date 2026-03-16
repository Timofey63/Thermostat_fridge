#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include <timerCallback.h>

Preferences prefs;

const char* ssid = "ESP32-FRIDGE";
const char* password = "12345678"; 

const byte DNS_PORT = 53;
DNSServer dnsServer;   
WebServer server(80);

#define OLED_SDA 5
#define OLED_SCL 6
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

const int ONE_WIRE_BUS = 10;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const int LED_PIN = 4;
int TEMP_OFF = -15;
int TEMP_INTERVAL = 2;
int TEMP_ON = TEMP_OFF + TEMP_INTERVAL;
unsigned long TIME_UPDATE = 5 * 60 * 1000;
int currentTemp;

bool compressorActive = false;

int getTempSensor()
{
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);
  currentTemp = tempC;

  return tempC;
}

void updateFridge()
{
  int temp = getTempSensor();
  Serial.print("temp= ");
  Serial.println(temp);

  if(compressorActive == false)
  {
    //warm
    if(temp >= TEMP_ON)
    {
      compressorActive = true;
    }
  }
  else //fridge on
  {
    if(temp <= TEMP_OFF)
    {
      //cool down
      compressorActive = false;
    }
  }

  digitalWrite(LED_PIN, compressorActive ? LOW : HIGH);
}

void printOled()
{
  u8g2.clearBuffer();

  u8g2.drawFrame(0, 0, 72, 40);

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(5, 12, ssid);
  u8g2.drawStr(5, 22, password);
  u8g2.setCursor(5, 32);

  //u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.print("Temp: ");
  u8g2.print(currentTemp);
  u8g2.print("C");

  u8g2.sendBuffer();
}

void handleRoot()
{
  Serial.println("=== handleRoot called ===");
  
  // Проверяем, смонтирована ли файловая система
  if (!LittleFS.begin(false)) { // false = не форматировать автоматически
    Serial.println("LittleFS not mounted!");
    server.send(500, "text/plain", "LittleFS not mounted");
    return;
  }
  
  // Проверяем, есть ли файл
  if (!LittleFS.exists("/index.html")) {
    Serial.println("index.html not found!");
    
    // Выводим список всех файлов для диагностики
    Serial.println("Files in LittleFS:");
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while(file) {
      Serial.print("  ");
      Serial.print(file.name());
      Serial.print(" - ");
      Serial.println(file.size());
      file = root.openNextFile();
    }
    
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
}

void serveCSS() 
{
    File file = LittleFS.open("/css/style.css", "r");
    if (!file) { server.send(404, "text/plain", "Not found"); return; }
    server.streamFile(file, "text/css");
    file.close();
}

void serveJS() 
{
    File file = LittleFS.open("/js/script.js", "r");
    if (!file) { server.send(404, "text/plain", "Not found"); return; }
    server.streamFile(file, "application/javascript");
    file.close();
}

void handleApi()
{
    DynamicJsonDocument doc(1024);
    doc["currentTemp"] = currentTemp;
    doc["tempOff"] = TEMP_OFF;
    doc["tempInterval"] = TEMP_INTERVAL;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleSendText1() 
{
    if (server.hasArg("value1")) {
        String value1 = server.arg("value1");
        TEMP_OFF = value1.toInt();
        TEMP_ON = TEMP_OFF + TEMP_INTERVAL;
        prefs.putInt("temp_off", TEMP_OFF);
        
        Serial.print("Temperature OFF set to: ");
        Serial.print(TEMP_OFF);
        Serial.println("°C");
        
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Value not received");
    }
}

void handleSendText2() 
{
    if (server.hasArg("value2")) {
        String value2 = server.arg("value2");
        TEMP_INTERVAL = value2.toInt();
        prefs.putInt("temp_interval", TEMP_INTERVAL);
        
        Serial.print("Update interval set to: ");
        Serial.print(TEMP_INTERVAL);
        Serial.println("C");
        
        server.send(200, "text/plain", "OK");
    } else {
        server.send(400, "text/plain", "Value not received");
    }
}

timerCallback ledTimer(updateFridge, TIME_UPDATE);
timerCallback oledTimer(printOled, 1000);

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  prefs.begin("settings", false);
  TEMP_OFF = prefs.getInt("temp_off", -15);
  TEMP_INTERVAL = prefs.getInt("temp_interval", 2);
  TEMP_ON = TEMP_OFF + TEMP_INTERVAL;

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!u8g2.begin()) 
  {
    Serial.println("OLED не найден! Проверьте подключение I2C.");
    while (1);
  }

  getTempSensor();

  WiFi.softAP(ssid, password);
  IPAddress apIP = WiFi.softAPIP();

  dnsServer.start(DNS_PORT, "*", apIP);
  dnsAdressSetup();
  
  // http://192.168.4.1/
  server.on("/css/style.css", serveCSS);
  server.on("/js/script.js", serveJS);

  server.on("/", handleRoot);
  server.on("/api", handleApi);
  server.on("/sendText", HTTP_GET, []() {
    if (server.hasArg("value1")) handleSendText1();
    if (server.hasArg("value2")) handleSendText2();
});

server.begin();
}

void loop()
{
  ledTimer.loop();
  oledTimer.loop();
  server.handleClient();
  dnsServer.processNextRequest();

  delay(10);
}