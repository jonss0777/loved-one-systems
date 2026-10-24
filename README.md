# LovedOne Systems
Smart home system to take care of our elderly


## System Architecture 

ESP32:
- Use the SMP arquitecture to run a thread for each sensor and give the wifi client its own core.
- Receives state configurations from the server.
- We will use LEDs, resistors, and buttons to represent a temperature sensor. We will press the button to switch between low, medium, high temperature the LEDs will help this changes in state. 
- The motion sensor will be represented  by the 3 axis sensor.
- CO2 Sensor to detect leaks of gas 
- Mic Sensor + camera 




Fast API: 
- Receives telemetric data
- Logs the data into a data base
- Updates system configuration
- Login/Logout

React Native: 
- Receive Notifications 
- Configure Sensor Thresholds 
- Visulize activity
- Live video camera
