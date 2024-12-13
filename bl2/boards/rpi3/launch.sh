#!/bin/bash



#qemu-system-aarch64 \
#

  /home/anders/qemu/qemu/build/qemu-system-aarch64 \
    -machine raspi3b \
    -cpu cortex-a53 \
    -m 1G -smp 4 \
    -dtb bcm2710-rpi-3-b-plus.dtb \
    -serial stdio \
    -S -s

   # -kernel kernel8.img \
