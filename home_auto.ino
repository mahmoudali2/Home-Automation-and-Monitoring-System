// Include the libraries
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

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign-in credentials */
#define AUTHOR_EMAIL "home.notifications.alerts@gmail.com"
#define AUTHOR_PASSWORD "ijlhqbjzhxkaoflf"

/* Recipient's email */
#define RECIPIENT_EMAIL "mahmoud.althaqel@gmail.com"

/* The SMTP Session object used for Email sending */
SMTPSession smtp;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

Session_Config session;
LiquidCrystal lcd(16, 4, 2, 0, 12, 13);

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

  /* Enable debug via Serial port */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Set the session config */
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = "Home Automation system";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "ESP Test Email";
  message.addRecipient("Mahmoud", RECIPIENT_EMAIL);

  /* Send HTML message */
  String htmlMsg = "<div style=\"color:#2f4468;\"><h1>The ESP board is connected!</h1><p>- Sent from ESP board</p></div>";
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  /* Connect to the server with the session config */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email: " + smtp.errorReason());

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
    if (flameStatus == HIGH) {
      Serial.print("Flame status: ");
      Serial.println("Flame detected");
      sendAlertEmail();
    } else {
      Serial.print("Flame status: ");
      Serial.println("No Flame");
    }
  }

  delay(2000);
}

void sendAlertEmail() {
  /* Declare the message class */
  SMTP_Message alertMessage;

  /* Set the message headers */
  alertMessage.sender.name = "Home Auto System";
  alertMessage.sender.email = AUTHOR_EMAIL;
  alertMessage.subject = "Fire Alert!";
  alertMessage.addRecipient("Mahmoud", RECIPIENT_EMAIL);

  /* Send HTML message */
  String htmlMsg = "<div style=\"color:#2f4468;\"><h1>Fire Alert!</h1><p>A fire has been detected in your home.</p></div>";
  alertMessage.html.content = htmlMsg.c_str();
  alertMessage.html.charSet = "us-ascii";
  alertMessage.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  /* Close the current session */
  smtp.closeSession();

  /* Create a new SMTP session */
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &alertMessage))
    Serial.println("Error sending Email: " + smtp.errorReason());
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status) {
  if (status.success()) {
    Serial.println("Message sent successfully.");
  } else {
    Serial.println("Error sending message: " + String(status.info()));
  }
}
