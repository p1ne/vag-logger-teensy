# vag-logger-teensy

This project is intended to dump selected CAN bus messages to SD card using Teensy 3.6 board.

This particular code is designed for Audi (A4B8/Q5 platform) to track RPM, cylinder misfires and cylinder ignition retardation angles. Misfires generate beeps using beeper wired to Analog output of Arduino.

## Libraries
* https://github.com/pawelsky/FlexCAN_Library - this one has timeouts for RX messages
* https://github.com/PaulStoffregen/SD

## Hardware used

* Teensy 3.6
* CAN differential to serial converter
* 5V voltage regulator
* OBD2 male and female connectors wired as pass-through

## Wiring

CAN0 alternative pins of Teensy are soldered to TXD and RXD pins of CAN converter
3.3V and GND pins of converter are soldered to respective voltage and GND pins of Teensy
CANH and CANL pins of CAN converter are soldered to pins 14 and 6 of OBD respectively

Beeper is wired to A2 and GND pins of Teensy

## CAN messages

Response messages:

All messages are 8 bytes long

7E8 05 62 20 0x6F rpm1 rpm2 - RPM message where RPM value is rpm1*256+rpm2

7E8 05 62 20 cyl ret1 ret2 - Retardation angles message, where cyl is 0A-0D for cylinders 1-4, and ret1*256+ret2 is signed int negative HEX value of angle

7E8 05 62 29 cyl misf1 misf2 - Misfire for last 1000 RPM message, where cyl is 1D-20 for cylinders 1-4 and misf1*256+misf2 is number of misfires for last 1000 RPM.

Request messages are the same, but with 7E0 instead of 7E8 and 0x55 in last 4 bytes. Message length is 8 bytes.

... the rest of messages are in the code ..

## Work logic

all messages but 0x7E8 are filtered by CAN masks/filters to lower CAN traffic 

messages are sent sequentally with pause of 120 msec inbetween

listener of messages is listening for response message within 30 msec

as there are unpredicted responses from another CAN tool I have plugged constantly in the car - CAN Sensor, messages are processed until listening queue is empty

0 misfires and 0 retards are not logged

MPI and FSI injectors activity is logged together with RPM in each line - and identified by the number of respective injections increasing from last time.


Logic to be fully implemented (as I had in can-dumper-sd project)
For misfires only 1st misfire is saved. If more misfires are coming within 1000 RPM - they are saved too. But in order to save log file size, we don't save subsequent messages with the same misfire value. Upon receiption of misfire message 1000 Hz 500msec long beep is generated through beeper

