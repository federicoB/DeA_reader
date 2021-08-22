De Agostini weather station data reader

Simple C program to read from the usb HID interface of the control unit.

Credits to Folly from MeteoNetwork forum for the original code (2011), I refactored, cleaned up and fixed some stuff.

Feel free to open an Issue if there are any problems.

The program must be run as superuser, it generates 5 csv for each sensor.

To check and install all dependencies:
```
sudo apt-get install build-essential cmake libusb-dev libusb-1.0-0 libusb-1.0-0-dev
```


To compile and run:
```
cmake .
make
sudo ./dea_reader
```
