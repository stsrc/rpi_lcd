# lcd_spi

bcm2708-rpi-b.dts

Topic of project: drivers for RPi to communicate with LCD TFT screen by SPI.

There were written 2 drivers, one for screen controller (ILI9341), and second 
for touchscreen controller (XPT2046). Controllers use seperate select (CS) lines.  

Drivers present usage of notification chain for communication between drivers.
Notification chain was used to turn on backlight when the screen is touched. 
Backlight is turned off after time interval (implicitly 10 sec).

To use this driver you must change device tree file in /boot directory:   
1. make backup copy in your source tree of bcm2708-rpi-b.dts file, i.e.:  
  cp arch/arm/boot/dts/bcm2708-rpi-b.dts arch/arm/boot/dts/OLD_bcm2708-rpi-b.dts  
2. move bcm2708-rpi-b.dts which is in repo (directory: device_tree) to dts directory  
3. compile file bcm2708-rpi-b.dts with command:  
  make bcm2708-rpi-b.dtb (or make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- bcm2708-rpi-b.dtb when cross-compiling)  
4. make backup copy of bcm2708-rpi-b.dtb file (cp /boot/bcm2708-rpi-b.dtb /boot/bcm2708-rpi-b.dtbOLD)  
5. copy arch/arm/boot/bcm2708-rpi-b.dtb to /boot/   

