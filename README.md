# CEC-gw2
A CEC gateway, running on the STM Discovery board. It containes tree interfaces:
1. The Cec wire connected to the cec pin of hdmi cable to a TV
2. A Sony A1 interface to control a 5.1 channel amp
3. a serial over USB for a computer, I use a mythtv frontend. This interface emulates the
   Pulse eight hw, so linux cec lib will work.
 
 The TV remote gets setup to send volume changes over the Sony A1 link to an external amp.
 The channel up/down and numbers etc... is sent to the mythfronted.
 
 I developed a small executive, in order to implement the low level bitbanger drivers without
 a poll loop.
 The actual program logic is implemented as user mode program reading byte strings from the drivers.
 
 The executive is also ported to an Ingenic jz4755. Since the ingenic has MMU support, some rearchitecture
 has been started but not ready, 
 The going-mips branch is actually the most active one.
