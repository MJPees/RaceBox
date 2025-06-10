#!/bin/bash
./esptool --chip esp32c3 --port /dev/ttyACM0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode keep --flash_freq keep --flash_size keep 0x0 ../bin/ESP32-C3/RaceBox-GhostCar.ino.bootloader.bin 0x8000 ../bin/ESP32-C3/RaceBox-GhostCar.ino.partitions.bin 0xe000 ../bin/ESP32-C3/boot_app0.bin 0x10000 ../bin/ESP32-C3/RaceBox-GhostCar.ino.bin
