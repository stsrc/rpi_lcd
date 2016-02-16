# lcd_spi

Topic of project: drivers for RPi to communicate with LCD TFT screen by SPI.

There were written 2 drivers, one for screen controller (ILI9341), and second 
for touchscreen controller (XPT2046). Controllers use seperate select (CS) lines.  

Drivers present usage of notification chain for communication between drivers.
Notification chain was used to turn on backlight when the screen is touched. 
Backlight is turned off after time interval (implicitly 10 sec).
