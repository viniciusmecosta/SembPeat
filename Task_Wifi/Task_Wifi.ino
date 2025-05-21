#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

TaskHandle_t wifiTaskHandle;
TaskHandle_t mqttTaskHandle;

void wifiTask(void *parameter) {
  WiFiManager wifiManager;
  wifiManager.autoConnect("ESP32_ConfigAP");

  Serial.println("WiFi conectado com sucesso: " + WiFi.SSID());
  vTaskDelete(NULL);
}

void mqttTask(void *parameter) {
  mqttClient.setServer(mqttServer, mqttPort);

  while (true) {
    if (WiFi.status() == WL_CONNECTED && !mqttClient.connected()) {
      Serial.println("Tentando conectar ao broker MQTT...");
      String clientId = "ESP32Client-" + String(random(0xffff), HEX);
      if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPassword)) {
        Serial.println("Conectado ao MQTT");
      } else {
        Serial.printf("Erro MQTT: %d\n", mqttClient.state());
      }
    }

    mqttClient.loop();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void publicarMensagem(const char* topico, const char* mensagem) {
  if (mqttClient.connected()) {
    mqttClient.publish(topico, mensagem);
    Serial.printf("Publicado: [%s] -> %s\n", topico, mensagem);
  } else {
    Serial.println("MQTT desconectado. Não foi possível publicar.");
  }
}

void setup() {
  Serial.begin(115200);

  xTaskCreatePinnedToCore(
    wifiTask,
    "WiFi Task",
    4096,
    NULL,
    1,
    &wifiTaskHandle,
    1
  );

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  xTaskCreatePinnedToCore(
    mqttTask,
    "MQTT Task",
    4096,
    NULL,
    1,
    &mqttTaskHandle,
    1
  );
}

void loop() {
}
