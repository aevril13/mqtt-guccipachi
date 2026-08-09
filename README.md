# mqtt-guccipachi

Shared MQTT setup for the XIAO ESP32C6 class, run by heejung. One Mosquitto
broker on heejung's laptop, two firmware sketches (LED control and A0 sensor,
kept **separate**), a WebSocket dashboard, and a skill students install to
control the boards.

```
broker/     Mosquitto config: TCP 1883 + WebSockets 9001, opened to the LAN
firmware/   MqttLed (LED control) and MqttSensor (A0 readings) - separate sketches
web/        dashboard.html - live view of every node, no build step
skills/     mqtt-guccipachi skill (SKILL.md + skill.sh) for students
```

## Install (students)

```bash
npx skills add aevril13/mqtt-guccipachi
```

That is the [skills.sh](https://www.skills.sh) CLI. It finds the skill in
`skills/mqtt-guccipachi/`, installs it to `~/.agents/skills/mqtt-guccipachi`,
and links it into your agent's skills directory. Add `-g` for a user-level
install, `-l` to list without installing.

Then:

```bash
cd ~/.agents/skills/mqtt-guccipachi
./skill.sh name <your-name>     # your board's name, saved to ~/.mqtt-guccipachi
./skill.sh check                # is the broker reachable?
./skill.sh devices              # which boards are online
./skill.sh led on
```

The name must match `DEVICE_NAME` in your board's `arduino_secrets.h`.

`skill.sh` needs the mosquitto clients — install from
<https://mosquitto.org/download/> (the Windows default path is detected
automatically). Run `skill.sh` from **Git Bash** on Windows.

You must be on the **ICEE WiFi** — the broker (`192.168.0.75`) is a private LAN
address and is not reachable from outside the classroom network.

## Flash a board

`firmware/MqttLed` and `firmware/MqttSensor` keep credentials out of the repo:

```bash
cd firmware/MqttLed            # or MqttSensor
cp arduino_secrets.example.h arduino_secrets.h   # then edit it
```

Set `DEVICE_NAME` to your name (the board's topic prefix). Leave it `""` and the
board falls back to `c6-` plus the last 3 bytes of its MAC.

```bash
arduino-cli lib install PubSubClient
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C6 firmware/MqttLed
arduino-cli upload -p COM3 --fqbn esp32:esp32:XIAO_ESP32C6 firmware/MqttLed
```

The board prints its name on the serial monitor at 115200 baud on boot.

## Dashboard

Open `web/dashboard.html` in a browser — single self-contained file, no server
and no build step. It connects to the broker over WebSockets (9001) and draws
each node's link to the broker, live A0 values, LED state, and a message log.
`dashboard.html?host=192.168.0.75` points it somewhere else.

It speaks MQTT over a raw WebSocket rather than loading mqtt.js from a CDN, so
it works on a classroom network with no internet access.

Read [`skills/mqtt-guccipachi/SKILL.md`](skills/mqtt-guccipachi/SKILL.md) for
the topic map, prerequisites, and troubleshooting.

## For the instructor (heejung)

### 1. Open the broker

Mosquitto 2.x binds to localhost only until a listener is declared, so a default
install accepts no LAN clients. In an **elevated** PowerShell:

```powershell
cd broker
powershell -ExecutionPolicy Bypass -File .\apply-broker-config.ps1
```

That backs up the existing config, installs `mosquitto.conf`, adds firewall
rules for 1883 and 9001 on the private profile, restarts the service, and prints
the LAN address to hand out.

> The broker laptop must stay on and connected to ICEE. Its IP (`192.168.0.75`)
> is what students put in `arduino_secrets.h` and the dashboard. If the IP
> changes, update it everywhere (or set `MQTT_HOST` / `?host=`).

### 2. Flash the boards

See "Flash a board" above — each student flashes `MqttLed` and/or `MqttSensor`
with their own `DEVICE_NAME`.

> Mosquitto 2.1 removed `log_dest eventlog`. If the service starts and
> immediately stops, that is the usual cause — the apply script prints the
> config parse error instead of leaving you guessing.

## Topics

| Topic | Direction | Payload |
|---|---|---|
| `guccipachi/<id>/led/set` | client → board | `on`, `off`, `toggle` |
| `guccipachi/<id>/led/state` | board → client | `on`, `off` (retained) |
| `guccipachi/<id>/sensor/a0` | board → client | `{"raw":2048,"mv":1650}` every 2s |
| `guccipachi/<id>/status` | board → client | `online`, `offline` (retained, last will) |

## Security

The broker runs with `allow_anonymous true` and no TLS. Anyone on the classroom
network can publish to any topic, including other students' boards. This is fine
for a lab on a trusted LAN and unacceptable anywhere else — do not expose port
1883 or 9001 to the internet.
