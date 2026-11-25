#!/bin/bash

PATH=/home/anders/mips/qemu-jz/qemu-JZ/mipsel-softmmu

$PATH/qemu-system-mipsel  -M pavo --mtdblock ./pavo-nand.bin -serial stdio -S -s
