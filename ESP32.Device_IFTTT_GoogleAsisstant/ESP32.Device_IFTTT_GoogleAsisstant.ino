#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6WGFVXtjN"
#define BLYNK_TEMPLATE_NAME "Home Automation Using Google Assistant"
#define BLYNK_AUTH_TOKEN "SQiSKdFgYU1Ij_w3t-S_0MhZERs3jXzq"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char auth[] = BLYNK_AUTH_TOKEN;

char ssid[] = "Winz 2G";
char pass[] = "12ba45678";
#define LED_PIN 12
#define MOTOR_PIN1 4
#define LED2_PIN 5
#define MOTOR_PIN2 15

void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);

  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_PIN1, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(MOTOR_PIN2, OUTPUT);
}

void loop() {

  digitalWrite(MOTOR_PIN2, LOW);
  Blynk.run();
}