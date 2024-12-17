#include <WiFi.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include "DHT.h"
#include <time.h>
#include <esp_wifi.h>

#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "pass"
#define DATABASE_URL "peat"
#define API_KEY "key"

#define DHT_PIN 27
#define DHT_TYPE DHT11
#define TRIGGER_PIN 33
#define ECHO_PIN 25
#define PIR_PIN 14

DHT dht(DHT_PIN, DHT_TYPE);
WiFiManager wifiManager;

void changeMACAddress() {
  uint8_t newMAC[] = {0x24, 0x0A, 0xC4, 0x00, 0x00, (uint8_t)random(0, 255)};
  esp_wifi_set_mac(WIFI_IF_STA, newMAC);
  Serial.print("New MAC Address: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", newMAC[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

bool attemptWiFiConnection(const char* ssid, const char* password, int retries = 5) {
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  int attempt = 0;

  while (WiFi.status() != WL_CONNECTED && attempt < retries) {
    delay(1000);
    Serial.print(".");
    attempt++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi connected!");
    return true;
  }
  Serial.println("\nFailed to connect.");
  return false;
}

void checkWiFiConnection() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("Attempting to connect to Wi-Fi...");

  if (attemptWiFiConnection(WIFI_SSID, WIFI_PASSWORD)) return;

  Serial.println("Failed with hardcoded Wi-Fi. Trying with a new MAC address...");
  changeMACAddress();
  if (attemptWiFiConnection(WIFI_SSID, WIFI_PASSWORD)) return;

  Serial.println("Trying saved WiFiManager credentials...");
  if (wifiManager.autoConnect("ESP_AutoConnect")) {
    Serial.println("WiFiManager connected!");
    return;
  }

  Serial.println("No Wi-Fi connected. Opening configuration portal...");
  wifiManager.startConfigPortal("ESP_AutoConnect");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected via WiFiManager!");
  } else {
    Serial.println("Continuing execution without Wi-Fi.");
  }
}

float readTemperature() {
  return dht.readTemperature();
}

float readHumidity() {
  return dht.readHumidity();
}

void printTemperatureAndHumidity() {
  float temperature = readTemperature();
  float humidity = readHumidity();
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C | Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}

void temperatureAndHumidityTask(void *pvParameters) {
  while (1) {
    printTemperatureAndHumidity();
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

float measureDistance() {
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  return duration * 0.034 / 2;
}

void printDistance() {
  float distance = measureDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
}

void distanceTask(void *pvParameters) {
  while (1) {
    printDistance();
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

void presenceTask(void *pvParameters) {
  while (1) {
    if (digitalRead(PIR_PIN) == HIGH) {
      Serial.println("Detected");

      delay(2800);
      unsigned long startMillis = millis();
      while (millis() - startMillis < 200) {  
        if (digitalRead(PIR_PIN) == HIGH) {
          Serial.println("Presence detected for 3 seconds");
          break;
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}



String getDateTime() {
  checkWiFiConnection();
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "2024-01-01T00:00:00Z";
  }
  
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(buffer);
}

void sendTemperatureAndHumidityToFirebase(float temp, float humidity, String timestamp) {
  checkWiFiConnection();
  Serial.println("Sending temperature and humidity to Firebase:");
  Serial.print("Temperature: ");
  Serial.print(temp);
  Serial.print(" °C | Humidity: ");
  Serial.print(humidity);
  Serial.print(" % | Timestamp: ");
  Serial.println(timestamp);
}

void sendDistanceToFirebase(float distance, String timestamp) {
  checkWiFiConnection();
  Serial.println("Sending distance to Firebase:");
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.print(" cm | Timestamp: ");
  Serial.println(timestamp);
}

void sendTemperatureAndHumidityTask(void *pvParameters) {
  while (1) {
    float temp = readTemperature();
    float humidity = readHumidity();
    String timestamp = getDateTime();
    sendTemperatureAndHumidityToFirebase(temp, humidity, timestamp);
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

void sendDistanceTask(void *pvParameters) {
  while (1) {
    float distance = measureDistance();
    String timestamp = getDateTime();
    sendDistanceToFirebase(distance, timestamp);
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  dht.begin();

  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  randomSeed(analogRead(0));

  checkWiFiConnection();

  xTaskCreate(temperatureAndHumidityTask, "Print_Temp_Hum", 2048, NULL, 2, NULL);
  xTaskCreate(distanceTask, "Print_Dist", 2048, NULL, 2, NULL);
  xTaskCreate(presenceTask, "Presence_Task", 1024, NULL, 5, NULL);
  xTaskCreate(sendTemperatureAndHumidityTask, "Send_Temp_Hum", 2048, NULL, 1, NULL);
  xTaskCreate(sendDistanceTask, "Send_Dist", 2048, NULL, 1, NULL);
}

void loop() {}
