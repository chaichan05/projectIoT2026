#include <Arduino.h>
#include <SPI.h>
#include <NewPing.h>
#include <Adafruit_ILI9341.h>

// ===== Ultrasonic =====
#define TRIG_PIN 4
#define ECHO_PIN 16
#define MAX_DISTANCE 200

#define TFT_CS 33
#define TFT_RST 32
#define TFT_DC 25

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// ===== Output =====
#define BUZZER_PIN 17
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
#define BUZZER_CHANNEL 0
#define BUZZER_FREQ 2000
#define BUZZER_RES 8



void setup()
{
  Serial.begin(115200);

  // ===== Buzzer setup =====
  ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RES);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWrite(BUZZER_CHANNEL, 0);

  // ===== TFT setup =====
  tft.begin();
  tft.setRotation(1); // แนวนอน
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextSize(4); // ✅ ใหญ่ขึ้น (ลองเปลี่ยนเป็น 3,4,5 ได้)
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
}

void drawDisplay(int distance) {
  uint16_t color = (distance > 0 && distance <= 50) 
                   ? ILI9341_RED 
                   : ILI9341_WHITE;

  tft.fillScreen(ILI9341_BLACK);

  tft.setTextSize(8);
  tft.setTextColor(color, ILI9341_BLACK);

  int digits = (distance <= 0) ? 3 : (distance < 10 ? 1 : (distance < 100 ? 2 : 3));
  int x = (320 - digits * 48) / 2;
  tft.setCursor(x, 60);

  if (distance <= 0) tft.print("---");
  else               tft.print(distance);

  tft.setTextSize(3);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setCursor(130, 175);
  tft.print("cm");
}

void loop()
{
  delay(100);
  int distance = sonar.ping_cm();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  drawDisplay(distance);

  if (distance > 0 && distance <= 50)
  {
    ledcWrite(BUZZER_CHANNEL, 180);
  }
  else
  {
    ledcWrite(BUZZER_CHANNEL, 0);
  }
}