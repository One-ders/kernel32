#!/bin/bash

qemu-system-aarch64 \
    -machine raspi3b \
    -cpu cortex-a53 \
    -m 1G -smp 4 \
    -serial stdio \
    -kernel kernel8.img \
    -dtb bcm2710-rpi-3-b-plus.dtb \
    -initrd rootf.img \
    -device loader,addr=0x80000,cpu-num=1 \
    -device loader,addr=0x80000,cpu-num=2 \
    -device loader,addr=0x80000,cpu-num=3 \
    -S -s

#
#  /home/anders/qemu/qemu/build/qemu-system-aarch64 \
#

#
#qemu-system-aarch64 \
#    -machine raspi3b \
#    -cpu cortex-a53 \
#    -m 1G -smp 4 \
#    -serial stdio \
#    -kernel kernel8.img \
#    -dtb bcm2710-rpi-3-b-plus.dtb \
#    -initrd rootf.img \
#    -S -s

#-drive file=rootf.img,format=raw,if=sd \
#    -device loader,file=kernel8.img,addr=0 \
#    -device loader,file=rootf.img,addr=0x02000000 \



