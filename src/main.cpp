#include <Arduino.h>
#include <NewPing.h>

// ===== Ultrasonic =====
#define TRIG_PIN 4
#define ECHO_PIN 16
#define MAX_DISTANCE 200

NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);

// ===== Output =====
#define BUZZER_PIN 17
#define LED_PIN    18  // LED บนบอร์ด ESP32

// ===== Buzzer PWM =====
#define BUZZER_CHANNEL 0
#define BUZZER_FREQ 2500
#define BUZZER_RES 8   // 0–255

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  // PWM สำหรับ ESP32
  ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RES);
  ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
  ledcWrite(BUZZER_CHANNEL, 0); // เงียบก่อน

   if (Serial.available() > 0) {  //ถ้าคอมพิวเตอร์ส่งข้อมูลมาใหจะทำใน if นี้
    int key = Serial.read();        //นำค่าที่คอมพิวเตอร์ส่งมาเก็บในตัวแปร key
    Serial.print("key : ");
    Serial.println(key);         //Arduino ส่งค่าในตัวแปร key เข้าคอมพิวเตอร์ Serial Monitor
  }
}
//wemos_d1_mini32
void loop() {
  delay(100);

  int distance = sonar.ping_cm();

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance > 0 && distance <= 100) {
    // ===== ใกล้ → ดัง =====
    digitalWrite(LED_PIN, HIGH);     // ใช้ดูตอนจำลอง
    ledcWrite(BUZZER_CHANNEL, 180);  // ของจริงดังยาว
  } else {
    digitalWrite(LED_PIN, LOW);
    ledcWrite(BUZZER_CHANNEL, 0);    // เงียบ
  }
}