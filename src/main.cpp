#include <Arduino.h>
#include <SPI.h>
#include <NewPing.h>
#include <Adafruit_ILI9341.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

// sensor Ultrasonic
#define TRIG_PIN 4
#define ECHO_PIN 16
#define MAX_DISTANCE 200

// จอแสดง
#define TFT_CS 33
#define TFT_RST 32
#define TFT_DC 25

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// ลำโพง
#define BUZZER_PIN 17
#define BUZZER_CHANNEL 0
#define BUZZER_FREQ 2000
#define BUZZER_RES 8

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

const char *ssid = "sannnnnk";
const char *password = "11111111";
WiFiClient wifiClient;

const char *mqttServer = "mqtt.netpie.io";
const int mqttPort = 1883;
const char *mqttClientId = "f2544c88-a796-4cdd-8103-d220c8674e7d";
const char *mqttUser = "zXAzatARkhQY5yATFWahrixnDPcQWnHg";
const char *mqttPassword = "eroXSscJr4eDj16e9MZLbzziAHKrsuwf";

const char *data_pub = "@shadow/data/update";
const char *command_sub = "@msg/blindassist/buzzer";

PubSubClient mqttClient(wifiClient);

#define PUBLISH_INTERVAL 2000
unsigned long lastPublish = 0;

// นับจำนวนครั้ง buzzer เตือน
bool buzzerWasOn = false;
int alertCount = 0;

// ===== ตัวแปรเสียงจากแอป =====
bool appBuzz = false;
unsigned long buzzStart = 0;

// ─────────────────────────────────────────
// MQTT รับคำสั่งจากแอป
void callback(char *topic, byte *payload, unsigned int length)
{
  String message;

  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  Serial.print("Message received: ");
  Serial.println(message);

  if (message == "buzzer")
  {
    Serial.println(">>> App button pressed");
    appBuzz = true;
    buzzStart = millis();
  }
}

// ─────────────────────────────────────────
void setup_wifi()
{
  Serial.print("Connecting WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected — IP: " + WiFi.localIP().toString());
}

// ─────────────────────────────────────────
int distanceCounter = 1;

void sendToFirebase(int distance, int alertCount)
{
  HTTPClient http;

  String url = "https://projectdist-3047d-default-rtdb.asia-southeast1.firebasedatabase.app/ultrasonicData.json";

  String distanceID = "distance" + String(distanceCounter++);

  String payload = "{ \"" + distanceID + "\": {";
  payload += "\"distance\": " + String(distance) + ",";
  payload += "\"alertCount\": " + String(alertCount);
  payload += "} }";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.PATCH(payload);

  if (httpResponseCode > 0)
  {
    Serial.println("Firebase response: " + http.getString());
  }
  else
  {
    Serial.println("Error sending to Firebase");
  }

  http.end();
}

// ─────────────────────────────────────────
void reconnectMQTT()
{
  while (!mqttClient.connected())
  {
    Serial.printf("Connecting MQTT %s:%d ...\n", mqttServer, mqttPort);

    if (mqttClient.connect(mqttClientId, mqttUser, mqttPassword))
    {
      Serial.println("---> MQTT connected");

      mqttClient.subscribe(command_sub);
    }
    else
    {
      Serial.printf("failed rc=%d, retry in 5s\n", mqttClient.state());
      delay(5000);
    }
  }
}

// ─────────────────────────────────────────
void publishData(int distance, int count)
{
  String msg = "{\"data\":{\"distance\":";
  msg += String(distance);
  msg += ",\"alertCount\":";
  msg += String(count);
  msg += "}}";

  Serial.println("Publish → " + msg);

  mqttClient.publish(data_pub, msg.c_str());
}

// ─────────────────────────────────────────
void drawDisplay(int distance)
{
  uint16_t color = (distance > 0 && distance <= 50)
                       ? ILI9341_RED
                       : ILI9341_WHITE;

  tft.fillScreen(ILI9341_BLACK);

  tft.setTextSize(8);
  tft.setTextColor(color);

  int digits = (distance <= 0) ? 3
               : (distance < 10) ? 1
               : (distance < 100) ? 2
                                  : 3;

  int x = (320 - digits * 48) / 2;

  tft.setCursor(x, 60);

  if (distance <= 0)
    tft.print("---");
  else
    tft.print(distance);

  tft.setTextSize(3);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(130, 175);
  tft.print("cm");
}

// ─────────────────────────────────────────
void setup()
{
  Serial.begin(115200);

  ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RES);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);

  tft.begin();
  tft.setRotation(1);

  setup_wifi();

  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}

// ─────────────────────────────────────────
void loop()
{
  if (!mqttClient.connected())
    reconnectMQTT();

  mqttClient.loop();

  int distance = sonar.ping_cm();

  Serial.print("Distance: ");
  Serial.println(distance);

  bool buzzerOn = (distance > 0 && distance <= 50);

  if (buzzerOn && !buzzerWasOn)
  {
    alertCount++;

    Serial.printf(">>> Alert! count = %d\n", alertCount);

    sendToFirebase(distance, alertCount);
  }

  buzzerWasOn = buzzerOn;

  drawDisplay(distance);

  // ===== เสียงจาก Ultrasonic =====
  if (buzzerOn)
  {
    ledcWriteTone(BUZZER_CHANNEL, 2000);
  }
  else
  {
    ledcWriteTone(BUZZER_CHANNEL, 0);
  }

  // ===== เสียงจากแอป =====
  if (appBuzz)
{
  ledcWriteTone(BUZZER_CHANNEL, 3000);

  if (millis() - buzzStart > 300)
  {
    appBuzz = false;
  }
}
else if (buzzerOn)
{
  ledcWriteTone(BUZZER_CHANNEL, 2000);
}
else
{
  ledcWriteTone(BUZZER_CHANNEL, 0);
}

  unsigned long now = millis();

  if (now - lastPublish >= PUBLISH_INTERVAL)
  {
    lastPublish = now;

    publishData(distance, alertCount);
  }
}