# MQTT to RF gateway

This repository explains how to create a small device capable of driving an RF
power outlet over WiFi using MQTT.

## Required materials

- Any Arduino board
- An ESP8266 module (it is a low-cost Wi-Fi microchip)
- A 433mhz wireless transmitter module (such as FS1000A)
- A 3.3V voltage regulator module (such as AMS1117) (needed because of the
  ESP8266 voltage)

> **Note:**
> Make sure to have the latest official firmware of your ESP module.
> The `esp8266` folder contains instructions howto install the proper firmware.


## Wiring

![wiring](schematics/schema_bb.png?raw=true "Wiring")

## Arduino sketch

Before upload the sketch into your arduino, you have to update some variables in
order to fit your setup:

File:
[mqtt-rf-with-esp/mqtt-rf-with-esp.ino](mqtt-rf-with-esp/mqtt-rf-with-esp.ino)

| Variable | Default | Description |
|----------|---------|-------------|
| `ssid`   | n/a     | Your WiFi SSID |
| `pass`   | n/a     | Your WiFi password |
| `mqtt_host` | (10.0.0.5) | Your MQTT host IP |
| `mqtt_port` | 1883 | Your MQTT host port |

You can now upload the program.

### Debugging

You can open the serial monitor (9600 bauds) to  see debug traces.

Debug traces can be disabled by setting the `DEBUG` constant to 0.
Removing debug traces makes the program lighter and a little faster.

## Protocol

The MQTT payload is formated like this: `X;AAAAA;BBBBB`

- `X`: 1 = ON and 0 = OFF
- `AAAAA`: the first part of your socket address.
  This is a binary string representing the DIP switch configuration of your
  power outlet.
  `1` if the the DIP switch is high, `0` otherwise.
- `BBBBB`: the second part of your socket address.

**Ex:**

If our power is configured like this:
`UP,DOWN,DOWN,DOWN,DOWN` - `DOWN,UP,UP,DOWN,DOWN` then the address will be:
`10000;01100`.

Send an ON signal: `1;10000;01100`

Send an OFF signal: `0;10000;01100`

## Play

Start a MQTT broker:

```bash
$ docker run -d -p 1883:1883 ncarlier/mqtt
```

Subscribe to the status topic (status messages sent by the Arduino):

```bash
$ docker run --rm -it --entrypoint="mosquitto_sub" ncarlier/mqtt \
  -h localhost \
  -t rf/status
```

Subscribe to the command result topic (result of a command):

```bash
$ docker run --rm -it --entrypoint="mosquitto_sub" ncarlier/mqtt \
  -h localhost \
  -t rf/command/result
```

Send an ON command to the Arduino/ESP module:

```bash
$ docker run --rm -it --entrypoint="mosquitto_pub" ncarlier/mqtt \
  -h localhost \
  -t "rf/command" \
  -m "1;11111;01000"
```

## What next?

Now you can control your RF power outlet using MQTT.

Here are some improvement ideas:

- Use Ethernet board to improve network reliability
- Add some sensors to publish extra data on another queue
- ...

---
