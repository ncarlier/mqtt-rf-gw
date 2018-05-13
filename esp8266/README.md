# ESP8266 Firmware

These files allows you to flash the ESP8266 module with the original firmware
(using AT commands).

## Prerequisite

Install [esptool.py](https://github.com/espressif/esptool).

Note:

If you are using Debian you may have to uninstall the official package of `pip`
and reinstall pip using `easy_install`.

```bash
$ sudo apt remove python-pip
$ sudo easy_install pip
$ sudo pip install esptool
```

## Hardware setup

Wiring of the ESP8266 module with FTDI USB to TTL serial adapter.

![wiring](schema_bb.png?raw=true "Wiring")

Connection is following:

- VCC: 3.3V (>300mA)
- GND: ground
- CH_PD: 3.3V
- RST: Leave floating or ground to reset
- GPIO0: ground
- U_TXD: RX of FTDI/Serial interface
- U_RXD: TX of FTDI/Serial interface

## Flashing the firmware

Install the NOBOOT firmware:

```bash
$ ./flash-noboot.sh
```

---
