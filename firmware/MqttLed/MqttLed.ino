// XIAO ESP32C6 MQTT LED node for the guccipachi classroom broker.
//
//   subscribes  guccipachi/<id>/led/set      payload "on" | "off" | "toggle"
//   publishes   guccipachi/<id>/led/state    "on" | "off"            (retained)
//   publishes   guccipachi/<id>/status       "online" | "offline"    (retained, LWT)
//
// <id> is DEVICE_NAME from arduino_secrets.h, or "c6-" + last 3 MAC bytes.
// A0 sensor publishing lives in the separate MqttSensor sketch.

#include <WiFi.h>
#include <PubSubClient.h>
#include "arduino_secrets.h"

#define LED_PIN 15                  // XIAO user LED, active LOW

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

String deviceId;
String topicLedSet, topicLedState, topicStatus;

void setLed(bool on) {
  digitalWrite(LED_PIN, on ? LOW : HIGH);
  mqtt.publish(topicLedState.c_str(), on ? "on" : "off", true);  // retained
  Serial.printf("LED -> %s\n", on ? "ON" : "OFF");
}

bool ledIsOn() {
  return digitalRead(LED_PIN) == LOW;
}

void onMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  msg.toLowerCase();

  Serial.printf("RX %s = %s\n", topic, msg.c_str());

  if (String(topic) != topicLedSet) return;

  if (msg == "on" || msg == "1" || msg == "true") {
    setLed(true);
  } else if (msg == "off" || msg == "0" || msg == "false") {
    setLed(false);
  } else if (msg == "toggle") {
    setLed(!ledIsOn());
  } else {
    Serial.printf("ignored payload: '%s'\n", msg.c_str());
  }
}

void connectWiFi() {
  Serial.printf("WiFi: connecting to %s", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Modem sleep parks downlink packets until the next DTIM beacon, which on a
  // busy classroom AP shows up as laggy LED commands. USB powered, so trade
  // the power saving for latency.
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  Serial.printf("\nWiFi: connected, IP %s, RSSI %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

void connectMqtt() {
  while (!mqtt.connected()) {
    Serial.printf("MQTT: connecting to %s:%d as %s ... ",
                  MQTT_HOST, MQTT_PORT, deviceId.c_str());

    // Last will: the broker publishes "offline" if this board drops off.
    bool ok = mqtt.connect(deviceId.c_str(),
                           nullptr, nullptr,
                           topicStatus.c_str(), 0, true, "offline");

    if (ok) {
      Serial.println("connected");
      mqtt.publish(topicStatus.c_str(), "online", true);
      // QoS 1: the broker holds the command until this board acknowledges it.
      mqtt.subscribe(topicLedSet.c_str(), 1);
      Serial.printf("subscribed to %s (qos 1)\n", topicLedSet.c_str());
      setLed(ledIsOn());  // republish current state so late subscribers see it
    } else {
      Serial.printf("failed rc=%d, retrying in 1s\n", mqtt.state());
      delay(1000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);   // start off
  delay(500);

  deviceId = DEVICE_NAME;
  if (deviceId.length() == 0) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char idBuf[16];
    snprintf(idBuf, sizeof(idBuf), "c6-%02x%02x%02x", mac[3], mac[4], mac[5]);
    deviceId = idBuf;
  }

  String base   = "guccipachi/" + deviceId;
  topicLedSet   = base + "/led/set";
  topicLedState = base + "/led/state";
  topicStatus   = base + "/status";

  Serial.printf("\n=== guccipachi MQTT LED node ===\ndevice id: %s\n", deviceId.c_str());
  Serial.printf("led set:   %s\n", topicLedSet.c_str());

  connectWiFi();

  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);
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
}
