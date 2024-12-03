# The-Arduino-Code
what was used in order to make the thing:
these were the resources i used:
- https://docs.arduino.cc/retired/library-examples/curie-ble/Genuino101CurieBLEHeartRateMonitor/
- https://how2electronics.com/pulse-rate-bpm-monitor-arduino-pulse-sensor/
- https://github.com/sparkfun/SparkFun_Bio_Sensor_Hub_Library  
- https://github.com/maximtrp/heart-sensor
- https://drive.google.com/file/d/121qHKYW9TaB6_tUbtgEeZ4WFFKlCWMK_/view
- https://github.com/megaconfidence/bt-heart-monitor

i also wrote the code first in Python and then i converted it to C language, reason why i did that was because i am more familiar with python and it was easier for me to write in it and then i converted the code to C language which took a bit of time. 

Some problems i encountered were:
- another thing that happened was that the serial monitor would not work sometimes and i had to look for the mistake and make sure all the libarys are connected and work, the same thing also happened for serial poltter which plots the heart rate, and i alo had a bit of problems with the bluetooth monitor so i did not know how to implement the code
- also some problems that appeared were was that the code took a long time to upload 
- the other problem was that when i connected the bluetooth module, and downloaded the terminal on the phone sometimes when i typed a message it would not appear in the computer or in the phone and that caused some problems which i had to fix and modify the code as well as the terminal on the phone
- the biggest porbalme  that happneded was that something happened with arduino uno and when i uploaded the code it would not send it to the arduino and arduino could not make commands, this was a very annoying problem and required me to chnage the arduino board as there was something wrong with it internally 

The materials that were used to create the project:
- Arduino IDE
- Pulse Sensor
- Breadboard
- Jumper Wires
- USB cable
- Arduino Uno
- Bluetooth HC=05


HOW EVERYTHING WORKS and how it was connected and why:
![image](https://github.com/user-attachments/assets/a431231e-3e63-4d1e-8978-ec5018fe4958)

okay the circuit explanation the connections and why they were connected this way:

THE PULSE SENSOR:
- The red wire  from the pulse sensor connected to Arduinos 5V, this is connected this way because for power supply so basically voltage and this is needed because the sensor needs to detect the tiny changes in the blood flow
- the black wire from the pulse sensor GND is connected to arduinos GND because for the  common ground reference and this is because that the current can flow properly and all the components can work together, THIS IS IMPORTANT BECAUSE WITHOUT THE GROUND THE SENSOR COULD NOT FUCNTION
- the purple single from the pulse sensor is the wire output of the sensor and what this means? well it connects to A0 on the arduino analog input pins and these pins are needed here bevause the sensor outputs varying voltage and not on or off signlas like the digital sensors

THE BLUETOOTH MODULE:
the connectiosn#
- GND --> Again the GND is the ground connection for the module, and the reason on why its connected to arduinos GND pin is to make sure that there is connection between the arduino and the bluetooth module
- VCC --> This is the power supply, its connected to arduinos 5V because it provides a stable 5V power
- TXD which is also transmit, so this basically transmits the data from the bluetooth to the arduino, and its connected to arduinos RX because this RX is configured to recieve data
- RXD which is the recieve, so this recieves the data from the arduino, and its connected to arduinos TX via a voltage divider, and this voltage divider basically reduces the arduinos 5V ouput to 3.3V this matches the HC-05 modules operating voltage

THE BREADBOARD:
- i used the breadboard because its neater so its easier toroubleshoot and modify the circuit and it also helps to manage multiple connections which are required for the sensor and the bluetooth

