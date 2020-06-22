This module sends an SPI message from a raspberry pi to an attached arduino.

Setup:
1) Compile the .dtb file into a device tree into a device tree blob overlay (.dtbo) using the dtc command.
Put the .dtbo file into /boot/overlays, and edit the /boot/config.txt file to include "dtoverlay=ARDUINO_SPI0"
This loads the device tree overlay file at boot.

2)Make/compile the kernel module and insert it.

3)Attach the arduino via it's appropriate connections and run arduino_code.ino

4)Write/read into the newly created file at /dev/ArduinoSpidev in order to send messages to the arduino.
