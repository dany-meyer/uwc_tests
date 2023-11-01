# Smart Container

The smart container's objective is to send GPS and temperature data to an MQTT broker (HiveMQ). To accomplish this, the code can be divided into components.

## 1. Arduino (ESP8266)

- The code is tested for this Arduino version.

## 2. WIFI Connection

- The WiFi connection is important to allow the data to be published to the MQTT broker.
- For details on the WiFi library, visit: [WiFiManager](https://github.com/tzapu/WiFiManager)

## 3. Temperature Sensor

- It uses a simple LM35 temperature sensor.
- For individual setup, refer to: [Temperature Sensor Setup](https://github.com/dany-meyer/uwc_tests/tree/main/NodeMCU_arduino_ide_temperature_sensor)

## 4. GPS Module

- For GPS, we use the neo6m or neo 7m modules, both should work okay.
- For individual setup, refer to: [GPS Setup](https://github.com/dany-meyer/uwc_tests/blob/main/basicGPS/basicGPS.ino)

## 5. MQTT

- For MQTT, we use HiveMQ private cluster. The starter code is provided in their documentation [here](https://hivemq.com/article/mqtt-on-arduino-nodemcu-esp8266-hivemq-cloud/).
- Note: It is important to follow the instructions carefully, especially regarding the certificates configuration.
