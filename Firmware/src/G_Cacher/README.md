# GNSS Sat Cacher Firmware so far

Clean modular Arduino firmware for the Tenstar ESP32-S3 GNSS Sat Cacher project.

## Main changes from the single-file sketch

- Split the firmware into small files by responsibility.
- Removed `handleSerialInput()` because the handheld device should use touch and the boot button.
- Kept Wi-Fi/server credentials as blank placeholders in `src/Config.h`.
- Added slower Wi-Fi/server retry backoff so a bad connection does not constantly interrupt the touch UI.
- Reduced display flicker by redrawing the full screen only when changing page.
- Live screen values use cached field updates, so a field is only cleared and redrawn when its text or colour changes.
- Backend `beep` now triggers one short target-reached melody once instead of repeatedly buzzing while the server keeps returning `beep: true`.

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

The zip deliberately leaves these blank.

## Required libraries

- LovyanGFX
- TinyGPSPlus
- Adafruit BMP280
- ArduinoJson
- The same QMI8658C `ImuDrv.hpp` driver used by your current sketch

## Notes

This was reorganised from the uploaded single-file project sketch. It has not been hardware-compiled in this environment because the Arduino ESP32 board package and your local sensor/display libraries are not installed here.
