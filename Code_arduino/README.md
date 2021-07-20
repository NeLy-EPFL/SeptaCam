# Arduino_Codes
Code for the arduino to control stimulation and to trigger frame acquisition from basler cameras.

Can be used with the GUI for the basler cameras.
If a new arduino is used with the GUI the udev rules have to be amended.
Use
`udevadm info -a -p $(udevadm info -q path -n /dev/ttyACM1)` to find the serial number of the arduino.
Then create a file `/etc/udev/rules.d/99-arduino-serial.rules`
with the following content:

```
ACTION=="add", ATTRS{idVendor}=="2a03", ATTRS{idProduct}=="0042", ATTRS{serial}=="85438333036351B04221", SYMLINK+="arduinoCams"

ACTION=="add", ATTRS{idVendor}=="2a03", ATTRS{idProduct}=="0042", ATTRS{serial}=="85633323430351612262", SYMLINK+="arduinoStim"
```

Replace the serial numbers accordingly.
