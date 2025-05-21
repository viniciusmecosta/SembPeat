#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT11

#define TRIGGER_PIN 12
#define ECHO_PIN 14

DHT dht(DHTPIN, DHTTYPE);

float readDistance() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  float distance = (duration / 2.0) * 0.0343;
  return distance;
}

float readTemperature() {
  return dht.readTemperature();
}

int readHumidity() {
  float hum = dht.readHumidity();
  if (isnan(hum)) return -1;
  return (int)(hum + 0.5);
}

void sensorTask(void *parameter) {
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  dht.begin();

  while (true) {
    float distance = readDistance();
    float temp = readTemperature();
    int hum = readHumidity();

    if (distance >= 0) Serial.printf("Distância: %.2f cm\n", distance);
    else Serial.println("Erro medindo distância");

    if (!isnan(temp)) Serial.printf("Temperatura: %.2f °C\n", temp);
    else Serial.println("Erro medindo temperatura");

    if (hum >= 0) Serial.printf("Umidade: %d %%\n", hum);
    else Serial.println("Erro medindo umidade");

    Serial.println("-----");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    sensorTask,
    "Sensor Task",
    4096,
    NULL,
    1,
    NULL,
    1
  );
}

void loop() {
}
