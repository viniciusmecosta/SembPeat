#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIR_PIN 13

bool presenceDetected = false;
unsigned long lastMotionTime = 0;
const unsigned long motionThreshold = 5000;

void printPresence() {
  Serial.println("Presença constante detectada!");
}

void motionTask(void *parameter) {
  pinMode(PIR_PIN, INPUT);

  while (true) {
    int state = digitalRead(PIR_PIN);

    if (state == HIGH) {
      lastMotionTime = millis();

      if (!presenceDetected) {
        presenceDetected = true;
      }
    } else {
      if (millis() - lastMotionTime > motionThreshold) {
        presenceDetected = false;
      }
    }

    if (presenceDetected && (millis() - lastMotionTime >= motionThreshold)) {
      printPresence();
      presenceDetected = false;
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    motionTask,
    "Motion Task",
    2048,
    NULL,
    1,
    NULL,
    1
  );
}

void loop() {
}
