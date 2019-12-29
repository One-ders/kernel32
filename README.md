# CEC-gw

This implements a Pulse8 emulator running on a STM 32 discovery board.

It is compatible with the libCEC on linux, and works with mythtv.

It is tested with sony and samsung tv's, Onkyo 608 amp, and a mythtv frontend.

There is also a driver for Sony A1 link protocol that can be used for old AV amps without
hdmi cec support, This code will in this case work as a gateway and present the amp as a cec amp
sending command to the amp via the A1 protocol. (volume up/down, and power on off).

The code comprises a set of functions:
1. a small OS (thread dispatcher since there is no mmu).
2. a set of drivers adhering to the driver api of the os:
  * CEC bit-banging driver to do the bit timeing of the CEC protocol.
  * Sony A1 bit-banging driver to do the bit timeing of the Sony A1 protocol
  * A USB hid serial interface driver to present it self as a pulse eight.
  * a rs232 serial driver for cli and troubleshooting.
  
3. the user land (non priviledged) code to do the high level job.

A stm-discovery compiler is needed...
This code has been tested with both the MB997C and MB1075B board.

To build, source the EnvSettings file in this directory, 
make sure you have the compiler at the path specified in the EnvSettings file.

then cd down to the directory for you board, if you have the MB997C

cd bl2/boards/MB997C/

do make

use the stlink program and gdb to load the discovery board.

MB997C is sold under the name: STM32F407G-DISC1  20 dollars on mouser.
MB1025B is sold under the name: STM32F429I-DISC1 has an LCD, and 1 Meg ram, ~30 dollars on mouser.


