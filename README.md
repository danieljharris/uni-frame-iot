# uni-frame-iot
This is a project for the ESP8266 to allow for a 'plug and play' REST network of devices.

The way it works is as follows:
1) The first ESP powerd up will create a WiFi access point for configuration, once connected you can select a WiFi network from the drop down menu and enter the passwor. This is how the master device will communicate with the internet.
2) The first ESP then connects to the given WiFi point, and creats a new access point for future ESP devices.
3) Any further ESP devices powered on will detect the first ESP, and get the WiFi login information from this, meaning you only need to enter you WiFi details once.
4) You can then access any device using the endpoints accessable at esp8266.local or via 192.168.1.100 (configurable in the code).

Some notes about how it works:
* All ESP devices are communicated to via the first powered on ESP (known as the master). Which device is the master is not relavant, as all devices are accessed the same.
* You communicate to the endpoints using JSON and you specify which ESP to use using its "name" (which can be configured via an endpoint).

# Libraries
Please note this project requires the following libraries at these specific versions (using newer versions will not work).
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson) @ 5.13.2
- [TinyUPnP](https://github.com/ofekp/TinyUPnP) @ 3.1.4

# Environment
This project is designed to be compiled using [PlatformIO](https://platformio.org/). To compile the project outside of PlatformIO (such as in the Arduino IDE) please move all files from /src and /include into a single folder.
