# AutoMag
C application that utilizes Carberry for OBD Query 

Hardware prerequisites: Raspberry Pi, Carberry module(http://carberry.it/), OBD2 connector.
Scheme for OBD2 pinout can be found here: http://opengarages.org/handbook/ebook/images/000046.jpg
with an addition that IGN_ON wire goes on PIN 16(12V). 

This application is used for OBD Query commands for the Skoda Octavia 3 A7 model.
Every 3 seconds two RPM bytes are printed on standard output.
The responses from the Car are saved into txt file.

Application is wrapped around libtelnet that can be found on the following link: 
https://github.com/seanmiddleditch/libtelnet
The only changes done ware on telnet-client.c file.

