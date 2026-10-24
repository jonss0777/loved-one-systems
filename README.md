# LovedOne Systems
Smart home system to take care of our elderly


## System Architecture 

ESP32:
- Receives sensor type configurations from the server.
- Can send state updates for different sensors.
- Buttons will help us switch between states for the temperature, c02, and 
- The temperature sensor will detect dagerous temperature levels.
- the CO2 sensor to detect leaks of gas.
- The motion sensor will detect if there wasn't any activity all day. For example not seeing someone in the living room all day atleast once during the day when they usually do might be a sign of an anomaly.
- Mic and camera. Instead of having the camera be on all the time it turns on with specific phrases or words. 


Drogon REST API and Websockets: 
- Receives telemetric data.
- Logs the data into a database.
- Updates system configuration.
- Receives hearbeats.
- Login/Logout(Optional if there is enough time)


React Native: 
- Receives notifications from the server.
- Configure sensor thresholds and sensor type.
- Display if the sensor is active.
- Visulize activity for the sensors.
- Live video camera 
