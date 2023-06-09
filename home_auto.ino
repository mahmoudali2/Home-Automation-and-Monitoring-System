
//ijlhqbjzhxkaoflf
// include the libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <FS.h>
#include <ArduinoJson.h>
#include <LiquidCrystal.h>
#include <ESP_Mail_Client.h>



const char* ssid = "Mahmoud iPhone";
const char* password = "neutrino";

const int rs = 16, en = 4, d4 = 2, d5 = 0, d6 = 12, d7 = 13;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#define DHTPIN 14       // D5 - Digital pin connected to the DHT sensor
#define flamePin 5      // D1 - Digital pin connected to the flame sensor

#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

float temperature = 0.0;
float humidity = 0.0;
int flameStatus = 0;

AsyncWebServer server(80);

void handleRoot(AsyncWebServerRequest* request) {
  if (SPIFFS.exists("/index.html")) {
    File file = SPIFFS.open("/index.html", "r");
    size_t fileSize = file.size();
    std::unique_ptr<char[]> buf(new char[fileSize]);
    file.readBytes(buf.get(), fileSize);
    String html = String(buf.get());
    html.replace("%TEMPERATURE%", String(temperature));
    html.replace("%HUMIDITY%", String(humidity));
    html.replace("%FLAME%", (flameStatus == 0) ? "No Flame" : "Flame Detected");
    request->send(200, "text/html", html);
  } else {
    request->send(404, "text/plain", "File Not Found");
  }
}

void handleData(AsyncWebServerRequest* request) {
  StaticJsonDocument<200> jsonDoc;
  jsonDoc["temperature"] = temperature;
  jsonDoc["humidity"] = humidity;
  jsonDoc["flameStatus"] = flameStatus;

  String jsonString;
  serializeJson(jsonDoc, jsonString);
  request->send(200, "application/json", jsonString);
}

String processor(const String& var) {
  return String();
}

void setup() {

  lcd.begin(16, 2);
  pinMode(flamePin, INPUT);
  Serial.begin(115200);
  dht.begin();
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
    lcd.setCursor(0, 0); 
    lcd.print("Connecting!..");
  }
  Serial.println(WiFi.localIP());
  lcd.setCursor(0, 1); 
  lcd.print(WiFi.localIP());

  if (!SPIFFS.begin()) {
    Serial.println("Failed to initialize SPIFFS");
    return;
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.begin();
}

void loop() {
  float newTemperature = dht.readTemperature();
  float newHumidity = dht.readHumidity();
  int newFlameStatus = digitalRead(flamePin);

  if (!isnan(newTemperature)) {
    temperature = newTemperature;
    Serial.print("Temperature: ");
    Serial.println(temperature);
    lcd.clear();
    lcd.setCursor(0, 0);  
    lcd.print("Temp: ");
    lcd.setCursor(6, 0);  
    lcd.print(temperature);
    lcd.print(" C");
  }

  if (!isnan(newHumidity)) {
    humidity = newHumidity;
    Serial.print("Humidity: ");
    Serial.println(humidity);  
    lcd.setCursor(0, 1);  
    lcd.print("Humidity: ");
    lcd.setCursor(10, 1); 
    lcd.print(humidity);
    lcd.print("%");
  }

  if (newFlameStatus == LOW || newFlameStatus == HIGH) {
    flameStatus = (newFlameStatus == LOW) ? 0 : 1;
    Serial.print("Flame Status: ");
    Serial.println(flameStatus);
  }

  delay(2000);
}
