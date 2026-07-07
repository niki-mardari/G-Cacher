# G-Cacher

**A portable GNSS surveying and location-mapping tool**

<p align="center">
  <img width="360" alt="G-Cacher physical device" src="docs/images/G-Cacher.jpg" />
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

G-Cacher is a student engineering project developed during the BIP26 programme. The project utilizes a portable GNSS device. A web platform for collecting, storing, and visualising location data.

---

## What it currently does:

Device:

* Reads live GNSS data from multiple satellite constellations
* Displays position, altitude, speed, satellite count, HDOP, and signal quality
* Logs selected GNSS data to an SD card
* Sends telemetry to a backend server

Webserver (Frontend/ Backend):

* Visualises recorded location points in a web dashboard on a map
* A GeoCache-style navigation game
* Display recorded latitude, longitude, and altitude points in 3D
* Saves and displays saved locations

---

## Device Interface

The device uses a touch-based menu system for viewing live data and interacting with the project features.

The main device menus include:

* Live GNSS — displays parsed position, altitude, speed, HDOP, satellite count, date, and time.
* Signal Quality — shows whether the current GNSS signal is good, usable, weak, or unavailable.
* NMEA / Logging — supports viewing GNSS data and saving records to SD card.
* Saved Location — records a location and sends it to the backend as a saved asset.
* System Status — shows device, sensor, SD card, and telemetry status.


<p align="center">
  <img width="360" alt="G-Cacher Interface" src="docs/images/G-Cacher_UI_1.jpg" />
</p>

---

## Web Platform

The web platform receives telemetry from the G-Cacher device and provides a simple interface for interacting with the collected location data.

It includes a frontend website and a FastAPI backend. The backend receives device telemetry, stores records, and provides endpoints for the frontend. The frontend allows users to view location data, play the GeoCache game, inspect saved assets, and visualise terrain points. Deploy easily with Docker.

<p align="center">
  <strong>SatCacher Home Page</strong><br>
  <img width="680" alt="SatCacher home page" src="docs/images/Home_Page_Frontend.png" />
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
  <img width="680" alt="GeoCache game" src="docs/images/GeoCacheGame.png" />
</p>

<p align="center">
  <strong>Sample Waypoint</strong><br>
  <img width="680" alt="Sample waypoint" src="docs/images/GeoCacheGame2.jpg" />
</p>

---

## 3D Terrain Viewer

The 3D Terrain Viewer displays recorded or simulated GNSS points using latitude, longitude, and altitude.

It can be used to visualise walking routes, field test data, campus areas, and rough terrain-like surfaces. Points can come from the FastAPI database, imported JSON files, or simulated data.

<p align="center">
  <strong>3D Terrain Viewer</strong><br>
  <img width="680" alt="3D terrain viewer" src="docs/images/1_3D_Viwer_Simulated.png" />
</p>

<p align="center">
  <strong>Sample Terrain</strong><br>
  <img width="680" alt="Sample terrain" src="docs/images/2_3D_Viewer_Simulated.png" />
</p>

<p align="center">
  <strong>TUD Tallaght Campus Terrain</strong><br>
  <img width="680" alt="TUD Tallaght campus terrain" src="docs/images/3_3D_Viewer_Tallaght_Campus.png" />
</p>

---

## Saved Locations

The Saved Locations feature allows the device to record an important location and send it to the backend as an asset.

On the web dashboard, saved points can be viewed on a map and given extra details such as a name, category, and description. This can be used for marking parked cars, survey points, field markers, underground pipes, cables, or other important positions that may need to be found again later.

<p align="center">
  <strong>Saved Locations</strong><br>
  <img width="680" alt="Saved locations page" src="docs/images/Saved_Locations.png" />
</p>

---

---

## Hardware

### Tenstar ESP32-S3

[![Manual](https://img.shields.io/badge/Manual-Blue?style=for-the-badge&logo=readthedocs&logoColor=white&color=1E90FF)](https://manuals.plus/ae/1005006590631744)
[![Purchase](https://img.shields.io/badge/Purchase-Green?style=for-the-badge&logo=aliexpress&logoColor=white&color=2EA44F)](https://www.aliexpress.com/item/1005006454900498.html#nav-description)

### LC29H(AA) GNSS Receiver

[![Manual](https://img.shields.io/badge/Manual-Blue?style=for-the-badge&logo=readthedocs&logoColor=white&color=1E90FF)](https://www.waveshare.com/wiki/LC29H%28XX%29_GPS/RTK_HAT)
[![Purchase](https://img.shields.io/badge/Purchase-Green?style=for-the-badge&logo=aliexpress&logoColor=white&color=2EA44F)](https://www.aliexpress.com/item/1005006000498473.html)

### ILI9488 4.0 Inch

[![Manual](https://img.shields.io/badge/Manual-Blue?style=for-the-badge&logo=readthedocs&logoColor=white&color=1E90FF)](https://www.alldatasheet.com/datasheet-pdf/view/1131840/ETC1/ILI9488.html)
[![Purchase](https://img.shields.io/badge/Purchase-Green?style=for-the-badge&logo=aliexpress&logoColor=white&color=2EA44F)](https://www.aliexpress.com/item/33015586094.html)
---

## Learning Portal / Wiki

[![Learn More - GNSS Learning Portal](https://img.shields.io/badge/LEARN%20MORE-GNSS%20Learning%20Portal-39C5BB?style=for-the-badge\&labelColor=00AEEF\&logo=githubpages\&logoColor=white)](https://coolsphere.github.io/GNSS_Satellite/)

The wiki contains the more detailed technical documentation for GNSS, NMEA, sensors, wiring, software setup, and project development.

---

## Future Improvements

Future improvements may include:

* Further documentation
* Sensor Fusion with extended Kalman filtering
* PCB
* Improved enclosure

---

## Contributors

* Andrei-Eduard Rusu
* Carla Daniela Zegarra Bernabe
* Niki Mardari
* Szymon Blazejowski

---
---

## Notes

Contributions, suggestions, and improvements are welcome.
