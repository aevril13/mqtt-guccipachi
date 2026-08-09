// XIAO ESP32C6 MQTT sensor node for the guccipachi classroom broker.
//
//   publishes   classroom/<id>/sensor/a0    {"raw":2048,"mv":1650}  every 2s
//   publishes   classroom/<id>/status       "online" | "offline"    (retained, LWT)
//
// <id> is DEVICE_NAME from arduino_secrets.h, or "c6-" + last 3 MAC bytes.
// LED control lives in the separate MqttLed sketch.

#include <WiFi.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"

#define A0_PIN  A0                  // D0 / GPIO0
const unsigned long PUBLISH_INTERVAL_MS = 2000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

String deviceId;
String topicSensor, topicStatus;
unsigned long lastPublish = 0;

void connectWiFi() {
  Serial.printf("WiFi: connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  Serial.printf("\nWiFi: connected, IP %s, RSSI %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

void connectMqtt() {
  while (!mqtt.connected()) {
    Serial.printf("MQTT: connecting to %s:%d as %s ... ",
                  MQTT_HOST, MQTT_PORT, deviceId.c_str());

    bool ok = mqtt.connect(deviceId.c_str(),
                           nullptr, nullptr,
                           topicStatus.c_str(), 0, true, "offline");

    if (ok) {
      Serial.println("connected");
      mqtt.publish(topicStatus.c_str(), "online", true);
    } else {
      Serial.printf("failed rc=%d, retrying in 1s\n", mqtt.state());
      delay(1000);
    }
  }
}

void publishSensor() {
  int raw = analogRead(A0_PIN);
  int mv  = analogReadMilliVolts(A0_PIN);

  char payload[64];
  snprintf(payload, sizeof(payload), "{\"raw\":%d,\"mv\":%d}", raw, mv);

  mqtt.publish(topicSensor.c_str(), payload);
  Serial.printf("TX %s = %s\n", topicSensor.c_str(), payload);
}

void setup() {
  Serial.begin(115200);
  pinMode(A0_PIN, INPUT);
  analogReadResolution(12);          // 0-4095
  delay(500);

  deviceId = DEVICE_NAME;
  if (deviceId.length() == 0) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char idBuf[16];
    snprintf(idBuf, sizeof(idBuf), "c6-%02x%02x%02x", mac[3], mac[4], mac[5]);
    deviceId = idBuf;
  }

  String base   = "classroom/" + deviceId;
  topicSensor   = base + "/sensor/a0";
  topicStatus   = base + "/status";

  Serial.printf("\n=== guccipachi MQTT sensor node ===\ndevice id: %s\n", deviceId.c_str());
  Serial.printf("sensor:    %s\n", topicSensor.c_str());

  connectWiFi();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(10);
  connectMqtt();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();

  if (!mqtt.connected()) {
    Serial.printf("MQTT: disconnected (state=%d, WiFi RSSI %d dBm)\n",
                  mqtt.state(), WiFi.RSSI());
    connectMqtt();
  }

  mqtt.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;
    publishSensor();
  }
}
