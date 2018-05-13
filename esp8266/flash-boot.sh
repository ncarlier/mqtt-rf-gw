#!/bin/bash

esptool.py --port /dev/ttyUSB0 write_flash \
  0x00000 boot_v1.5.bin \
  0x01000 user1.1024.new.2.bin \
  0xfc000 esp_init_data_default.bin \
  0x7e000 blank.bin

