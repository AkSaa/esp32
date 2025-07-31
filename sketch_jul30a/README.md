# Project: sketch_jul30a

## Description
ESP32 temperature sensor for the Sauna, MQTT publish to Home Assistant with autodetect and sleep. 

## Required Libraries

- OneWire.h
- DallasTemperature.h
- WiFi.h
- freertos/FreeRTOS.h
- freertos/timers.h
- AsyncMQTT_ESP32.h
- arduino_secrets.h
- Preferences.h
- OneWire
- DallasTemperature

## How to Install Libraries

Arduino Library Manager:
- Async TCP by ESP32Async v3.4.6
- AsyncMQTT_ESP32 by Marvin ROGER, Khoi Hoang v1.10.0
- DallasTemperature by Miles Burton v4.0.3
- ESP Async WebServer by ESP32Async v3.7.10
- OneWire [v2.3.8](https://www.pjrc.com/teensy/td_libs_OneWire.html)
- WebServer_ESP32_enc by Khoi Hoang v1.5.3
- WebServer_ESP32_SC_enc by Khoi Hoang v1.2.1
- WebServer_ESP32_SC_W5500 by Khoi Hoang v1.2.1
- WebServer_ESP32_SC_W6100 by Khoi Hoang v1.2.1
- WebServer_ESP32_W5500 by Khoi Hoang v1.2.1
- WebServer_ESP32_W6100 by Khoi Hoang v1.2.1
- WebServer_WT32_ETH01 by Khoi Hoang v1.2.1

## Usage

1. Copy or clone this folder to your Arduino projects directory.
2. Open the `.ino` file in the Arduino IDE.
3. Make sure the required libraries are installed.
4. Select the correct board and port.
5. Upload the sketch.

## Notes

#define LED_BUILTIN 8 
    ESP32-C3-SuperMini inbuild LED
#define ONE_WIRE_BUS 3
    DallasTemperature Sensor installed to GPIO3 


In code, adjust as needed:
```
void waitTemperature() {
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);

  uint64_t sleepMinutes;
  if (tempC >= 20.0 && tempC <= 80.0) { // Adjust if needed
    sleepMinutes = 1;  // Rising temperature, short sleep. Adjust as needed.
  } else {
    sleepMinutes = 10; // Normal temperature, longer sleep. Adjust as needed.
  }
```