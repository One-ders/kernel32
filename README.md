
Make sure the container get started with X support

xhost +
podman run -v /tmp/.X11-unix:/tmp/.X11-unix -it --rm docker.io/nders/mips-env bash

Quick start:
1. source Envsettings.mips
2. cd bl2/boards/pavo
3. make
4. cd
5. pavo_nandflash
#if container started with X support
6. qemu-system-mipsel -M pavo -mtdblock pavo-nand.bin
#else container started without X support
6. qemu-system-mipsel -M pavo -mtdblock pavo-nand.bin -nographic





# CEC-gw

The project started out as a Hdmi Pulse8 CEC emulator for the STM discovery board.
(that project still lives under the branch tag discovery-cec-gw).

The master branch is now used for further development of the small OS in the previous project, but here it is
targeting an Mips processor from Ingenic, commonly used in small internet radios sold a decade ago.
Since it has an mmu, the Kernel part is more fun.
