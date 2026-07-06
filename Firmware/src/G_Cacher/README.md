# GNSS Sat Cacher Firmware so far

Clean modular Arduino firmware for the Tenstar ESP32-S3 GNSS G Cacher project

## Files

```text
GNSS_Sat_Cacher_Clean/
├── GNSS_Sat_Cacher_Clean.ino
└── src/
    ├── AppState.h
    ├── Buzzer.h / Buzzer.cpp
    ├── Config.h
    ├── GnssManager.h / GnssManager.cpp
    ├── Hardware.h / Hardware.cpp
    ├── InputManager.h / InputManager.cpp
    ├── NetworkManager.h / NetworkManager.cpp
    ├── SensorManager.h / SensorManager.cpp
    ├── StorageManager.h / StorageManager.cpp
    ├── UiManager.h / UiManager.cpp
    └── Utils.h / Utils.cpp
```

## Where to edit settings

Open `src/Config.h` and fill in only your local private values:

```cpp
const char *const WIFI_SSID = "";
const char *const WIFI_PASSWORD = "";
const char *const SERVER_HOST = "";
const char *const TELEMETRY_URL = "";
const char *const ASSETS_URL = "";
```

## Required libraries

- LovyanGFX
- TinyGPSPlus
- Adafruit BMP280
- ArduinoJson
- QMI8658C `ImuDrv.hpp` driver used by accelerometer can be found here: 
[![Lib](https://img.shields.io/badge/Manual-Purple?style=for-the-badge&logo=readthedocs&logoColor=white&color=1E90FF)]([https://www.waveshare.com/wiki/LC29H%28XX%29_GPS/RTK_HAT](https://github.com/lewisxhe/SensorLib/tree/master/src))
