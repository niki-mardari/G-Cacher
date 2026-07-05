# G-Cacher

**A portable GNSS surveying and location-mapping tool built with an ESP32-S3, LC29H GNSS receiver, touch display, SD logging, and a FastAPI web platform.**

<p align="center">
  <img width="360" alt="G-Cacher device concept" src="https://github.com/user-attachments/assets/1a463576-31dd-4d96-840d-1e0fd5fb8bca" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/GNSS-Surveying-1E90FF?style=for-the-badge" />
  <img src="https://img.shields.io/badge/ESP32--S3-Device-404040?style=for-the-badge&logo=espressif&logoColor=white" />
  <img src="https://img.shields.io/badge/LC29H-GNSS%20Receiver-2EA44F?style=for-the-badge" />
  <img src="https://img.shields.io/badge/FastAPI-Backend-009688?style=for-the-badge&logo=fastapi&logoColor=white" />
  <img src="https://img.shields.io/badge/Web-Dashboard-F7DF1E?style=for-the-badge&logo=javascript&logoColor=black" />
</p>

---

## Overview

G-Cacher is a student engineering maker project developed for the BIP26 programme. It combines a portable GNSS device with a web platform for collecting, storing, and visualising location data.

The physical device reads satellite positioning data from an LC29H GNSS receiver, displays live readings on an ILI9488 touch screen, logs useful data to an SD card, and can send telemetry to a FastAPI backend over Wi-Fi.

The web platform extends the device with tools for location-based games, 3D point visualisation, saved location management, and API testing.

This project demonstrates embedded systems, GNSS positioning, telemetry, backend development, frontend development, and practical enclosure design in one complete engineering prototype.

---

## Contributors

* Andrei-Eduard Rusu
* Carla Daniela Zegarra Bernabe
* Niki Mardari
* Szymon Blazejowski

---

## What G-Cacher Can Do

* Read live GNSS data from multiple satellite constellations
* Display position, altitude, speed, satellite count, HDOP, and signal quality
* Log selected GNSS data to an SD card
* Send telemetry from the ESP32-S3 device to a backend server
* Store saved locations as named assets
* Visualise recorded location points in a web dashboard
* Support a GeoCache-style navigation game
* Display recorded latitude, longitude, and altitude points in 3D
* Provide API documentation for testing backend endpoints

---

## Physical Device

The G-Cacher device is built around a Tenstar ESP32-S3 board and an LC29H GNSS receiver.

Main hardware includes:

* Tenstar ESP32-S3 development board
* LC29H(AA) GNSS receiver
* ILI9488 touch display
* SD card storage
* BMP280 pressure and temperature sensor
* QMI8658C IMU for acceleration and rotation data
* 2000 mAh 3.7 V rechargeable Li-ion battery
* 3D printed enclosure

The device is designed to be portable and suitable for field testing. The battery target is up to approximately 5 hours under heavy use, depending on display brightness, Wi-Fi activity, GNSS usage, and logging behaviour.

<p align="center">
  <img width="360" alt="G-Cacher physical device" src="https://github.com/user-attachments/assets/d02fb363-c22b-4e05-8810-f08625705583" />
</p>

---

## Device Interface

The device uses a touch-based menu system for viewing live data and interacting with the project features.

The main device menus include:

* **Live GNSS** — displays parsed position, altitude, speed, HDOP, satellite count, date, and time.
* **Signal Quality** — shows whether the current GNSS signal is good, usable, weak, or unavailable.
* **NMEA / Logging** — supports viewing GNSS data and saving records to SD card.
* **Saved Location** — records a location and sends it to the backend as a saved asset.
* **System Status** — shows device, sensor, SD card, and telemetry status.

---

## GNSS Data

G-Cacher receives satellite positioning data through the LC29H GNSS module. The receiver can work with constellations such as GPS, GLONASS, Galileo, BeiDou, and QZSS, depending on configuration and signal availability.

The firmware parses useful values from the receiver, including:

* Latitude
* Longitude
* Altitude
* Speed
* Date and time
* Satellite count
* HDOP
* Fix quality
* Signal quality

Detailed GNSS and NMEA explanations are documented in the project wiki and supporting documentation.

---

## Signal Quality and Sensor Expansion

G-Cacher converts GNSS reception data into simple signal-quality states so the user can quickly understand whether the current reading is reliable.

The ESP32-S3 board also includes onboard sensors such as an IMU and BMP280 pressure sensor. These sensors can support future sensor-fusion work, where GNSS, motion, pressure, and altitude readings are combined to improve device awareness.

The project also explores Galileo OSNMA where supported. OSNMA can help authenticate Galileo navigation messages and improve resistance to some spoofing scenarios. It does not prevent RF jamming and should be treated as an additional signal-integrity feature rather than a complete security solution.

---

## Web Platform

The web platform receives telemetry from the G-Cacher device and provides a simple interface for interacting with the collected location data.

It includes a frontend website and a FastAPI backend. The backend receives device telemetry, stores records, and provides endpoints for the frontend. The frontend allows users to view location data, play the GeoCache game, inspect saved assets, and visualise terrain points.

<p align="center">
  <strong>SatCacher Home Page</strong><br>
  <img width="680" alt="SatCacher home page" src="https://github.com/user-attachments/assets/35294646-b04f-4075-a990-ca09a230cc95" />
</p>

---

## GeoCache Game

The GeoCache Game is a location-based feature designed for tourists or first-time visitors exploring Ireland.

The backend stores predefined landmarks. The G-Cacher device sends the user’s current location to the server, and the web platform calculates whether the user is getting closer to or further away from the target.

The device can then give simple direction feedback:

* **Red** — getting closer
* **Blue** — moving further away
* **Yellow** — not enough movement detected yet

<p align="center">
  <strong>GeoCache Game</strong><br>
  <img width="680" alt="GeoCache game" src="https://github.com/user-attachments/assets/05cf1eac-5227-4aa5-a85f-df75aafeabcc" />
</p>

<p align="center">
  <strong>Sample Waypoint</strong><br>
  <img width="680" alt="Sample waypoint" src="https://github.com/user-attachments/assets/dd6d2024-0da8-4e26-84df-cb277928729b" />
</p>

---

## 3D Terrain Viewer

The 3D Terrain Viewer displays recorded or simulated GNSS points using latitude, longitude, and altitude.

It can be used to visualise walking routes, field test data, campus areas, and rough terrain-like surfaces. Points can come from the FastAPI database, imported JSON files, or simulated data.

<p align="center">
  <strong>3D Terrain Viewer</strong><br>
  <img width="680" alt="3D terrain viewer" src="https://github.com/user-attachments/assets/ae58facb-ceb9-45d1-ad8f-b71f016d5482" />
</p>

<p align="center">
  <strong>Sample Terrain</strong><br>
  <img width="680" alt="Sample terrain" src="https://github.com/user-attachments/assets/cc376ff5-f4f1-41e6-b9a4-bbdf83a2dcf4" />
</p>

<p align="center">
  <strong>TUD Tallaght Campus Terrain</strong><br>
  <img width="680" alt="TUD Tallaght campus terrain" src="https://github.com/user-attachments/assets/ba3be553-182c-4fd5-b9de-3b4d795bf732" />
</p>

---

## Saved Locations

The Saved Locations feature allows the device to record an important location and send it to the backend as an asset.

On the web dashboard, saved points can be viewed on a map and given extra details such as a name, category, and description. This can be used for marking parked cars, survey points, field markers, underground pipes, cables, or other important positions that may need to be found again later.

<p align="center">
  <strong>Saved Locations</strong><br>
  <img width="680" alt="Saved locations page" src="https://github.com/user-attachments/assets/97315fad-9da7-441b-9735-3e7822f29f4b" />
</p>

<p align="center">
  <strong>Backend Data Storage</strong><br>
  <img width="680" alt="Database contents" src="https://github.com/user-attachments/assets/8bedb7af-100b-4a19-bb15-da6baa52aa40" />
</p>

---

## API Documentation

The backend includes automatically generated FastAPI documentation for testing implemented endpoints.

The API supports areas such as:

* Device telemetry
* Latest location data
* Saved locations
* Track data
* 3D point data
* Asset creation and editing

---

## Hardware

### Tenstar ESP32-S3

[![Manual](https://img.shields.io/badge/Manual-Blue?style=for-the-badge\&logo=readthedocs\&logoColor=white\&color=1E90FF)](https://manuals.plus/ae/1005006590631744)
[![Purchase](https://img.shields.io/badge/Purchase-Green?style=for-the-badge\&logo=aliexpress\&logoColor=white\&color=2EA44F)](https://www.aliexpress.com/item/1005006454900498.html#nav-description)

### LC29H(AA) GNSS Receiver

[![Manual](https://img.shields.io/badge/Manual-Blue?style=for-the-badge\&logo=readthedocs\&logoColor=white\&color=1E90FF)](https://www.waveshare.com/wiki/LC29H%28XX%29_GPS/RTK_HAT)
[![Purchase](https://img.shields.io/badge/Purchase-Green?style=for-the-badge\&logo=aliexpress\&logoColor=white\&color=2EA44F)](https://www.aliexpress.com/item/1005006000498473.html)

---

## Repository Structure

```text
G-Cacher/
├── firmware/       # ESP32-S3 firmware and Arduino sketches
├── web/            # Frontend and FastAPI backend
├── models/         # 3D enclosure and box design files
├── docs/           # Technical notes, wiring guides, and wiki material
├── README.md
└── .gitignore
```

---

## Learning Portal / Wiki

[![Learn More - GNSS Learning Portal](https://img.shields.io/badge/LEARN%20MORE-GNSS%20Learning%20Portal-39C5BB?style=for-the-badge\&labelColor=00AEEF\&logo=githubpages\&logoColor=white)](https://coolsphere.github.io/GNSS_Satellite/)

The wiki contains the more detailed technical documentation for GNSS, NMEA, sensors, wiring, software setup, and project development.

---

## Future Improvements

Future improvements may include:

* Improved enclosure design
* More complete field testing
* Better signal-quality mapping
* Sensor fusion using GNSS, IMU, and barometric readings
* Improved telemetry error handling
* Expanded OSNMA testing where supported
* More complete frontend styling and user workflows

---

## Notes

Contributions, suggestions, and improvements are welcome.
