# drivers
Device drivers

chatbird
--------

Linux driver for the Mitsumi PC Mascot or Chatbird.
Uses ALSA for the speaker.  Controls for wings and head TBD.

Rough summry of what I have been able to reverse angineer:

Sound is 16 bit 16kHz signed linear PCM split into 44-byte (22 word) packets.
Sound frames are sent using interrupt write.

Motors are operated using control messages of type 0x40 (custom) request 1.

tcpdump showed the following requests:
value 0xaf05, index 0x0037
value 0xbc0X, index 0x1388 where X=1,3 or 5.
Bottom 13 bits of index define motor duration.  4096=16 seconds.
value seems to be arranged as follows:
Bit 15 (MSB) must be set for motor operation.
Bit 7 enables the red LED
Bit 6 enables the green LED
Bottom 6 LSBs seem to be some sort of mode selector:
0 : nothing
1 : nothing
2 : nothing
3 : beak chatter
4 : head tilt
5 : wing flap
6 : nothing
7 : nothing
8 and above: nothing.

chatbird-tools
--------------

This will be for utilities that I write for the driver.

speak is a simple speech synthesis program that mimics the original idea of the device (to a certain extent).
testmotors sends ioctls for testing.  Currently requires a rebuild for each command.
