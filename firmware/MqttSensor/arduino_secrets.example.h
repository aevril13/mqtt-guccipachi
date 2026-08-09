// Copy to arduino_secrets.h and fill WIFI_PASS + DEVICE_NAME.
// arduino_secrets.h is gitignored so credentials never reach the repo.
#pragma once

#define WIFI_SSID   "ICEE"                 // 2.4GHz only - ESP32C6 has no 5GHz radio
#define WIFI_PASS   "YOUR_WIFI_PASSWORD"

#define MQTT_HOST   "192.168.0.75"         // heejung's laptop on ICEE (the broker)
#define MQTT_PORT   1883

// Your board's name on the broker: topics become classroom/<name>/...
// Leave it empty ("") to fall back to "c6-" + last 3 bytes of the MAC.
#define DEVICE_NAME "your-name"
