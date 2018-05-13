#!/bin/bash

esptool.py --port /dev/ttyUSB0 write_flash \
  0x00000 noboot/eagle.flash.bin \
  0x10000 noboot/eagle.irom0text.bin \
  0x7e000 blank.bin \
  0xfe000 blank.bin

