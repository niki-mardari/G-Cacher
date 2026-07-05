# G-Cacher

**A portable GNSS surveying, logging, and location-mapping tool built with an ESP32-S3, LC29H GNSS receiver, touch display, local SD storage, and a FastAPI web platform.**

<p align="center">
  <img width="430" alt="G-Cacher device" src="https://github.com/user-attachments/assets/1a463576-31dd-4d96-840d-1e0fd5fb8bca" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/GNSS-Surveying-1E90FF?style=for-the-badge" />
  <img src="https://img.shields.io/badge/ESP32--S3-Firmware-404040?style=for-the-badge&logo=espressif&logoColor=white" />
  <img src="https://img.shields.io/badge/LC29H-GNSS%20Receiver-2EA44F?style=for-the-badge" />
  <img src="https://img.shields.io/badge/FastAPI-Backend-009688?style=for-the-badge&logo=fastapi&logoColor=white" />
  <img src="https://img.shields.io/badge/JavaScript-Frontend-F7DF1E?style=for-the-badge&logo=javascript&logoColor=black" />
  <img src="https://img.shields.io/badge/Arduino-Firmware-00979D?style=for-the-badge&logo=arduino&logoColor=white" />
</p>

---

## Overview

G-Cacher is a student engineering maker project developed for the BIP26 programme. It combines embedded systems, GNSS satellite positioning, local data logging, wireless telemetry, backend storage, and a browser-based web interface.

The physical device reads raw NMEA data from an LC29H GNSS receiver and parses useful positioning information such as latitude, longitude, altitude, time, date, satellite count, HDOP, speed, and signal quality. The parsed readings are displayed on an ILI9488 touch display and can also be logged to an SD card.

The ESP32-S3 connects to a mobile hotspot and sends telemetry to a FastAPI backend. The web platform then stores, visualises, and manages the collected location data through features such as a GeoCache game, 3D terrain viewer, saved location manager, and API documentation.

This project is intended as a practical demonstration of GNSS-based surveying, field data collection, and location-aware web applications.

---

## Members / Contributors

* Andrei-Eduard Rusu
* Carla Daniela Zegarra Bernabe
* Niki Mardari
* Szymon Blazejowski

---

## Main Features

| Area                 | Description                                                                                      |
| -------------------- | ------------------------------------------------------------------------------------------------ |
| GNSS Device          | Portable ESP32-S3 device for reading, parsing, displaying, and logging GNSS data                 |
| Touch Interface      | ILI9488 touch display used for live readings, signal quality, saved locations, and device status |
| SD Card Logging      | Local storage for GNSS records such as coordinates, date, time, HDOP, and signal quality         |
| Wireless Telemetry   | ESP32-S3 sends GNSS data to a backend server over Wi-Fi using a mobile hotspot                   |
| FastAPI Backend      | Receives telemetry, stores data, and exposes API endpoints for the frontend                      |
| Web Frontend         | Provides tools for geocaching, 3D terrain viewing, saved locations, and API documentation        |
| 3D Printed Enclosure | Custom physical box design for holding the electronics and display                               |

---

## Physical Device

The G-Cacher device is built around the Tenstar ESP32-S3 development board and an LC29H GNSS receiver.

| Component                     | Purpose                                                                    |
| ----------------------------- | -------------------------------------------------------------------------- |
| Tenstar ESP32-S3              | Main microcontroller, Wi-Fi telemetry, firmware logic, and display control |
| LC29H(AA) GNSS Receiver       | Receives satellite positioning data and outputs NMEA strings               |
| ILI9488 Touch Display         | Shows GNSS readings, menus, status pages, and interaction screens          |
| SD Card Slot                  | Saves GNSS logs locally for later review or import                         |
| BMP280 Sensor                 | Measures pressure and temperature                                          |
| QMI8658C IMU                  | Measures acceleration and rotation                                         |
| 2000 mAh 3.7 V Li-ion Battery | Rechargeable portable power source                                         |
| 3D Printed Enclosure          | Holds and protects the device hardware                                     |

The battery target is up to approximately 5 hours under heavy use, including display activity, Wi-Fi telemetry, GNSS reception, and SD logging. Actual runtime depends on brightness, refresh rate, wireless activity, and firmware behaviour.

<p align="center">
  <img width="430" alt="G-Cacher physical device" src="https://github.com/user-attachments/assets/d02fb363-c22b-4e05-8810-f08625705583" />
</p>

---

## Device Interface

The firmware uses a touch-based menu system on the ILI9488 display.

| Menu Page      | Purpose                                                                                            |
| -------------- | -------------------------------------------------------------------------------------------------- |
| Live GNSS      | Displays parsed latitude, longitude, altitude, ground speed, HDOP, satellite count, date, and time |
| Signal Quality | Maps GNSS reception into simple quality categories such as good, OK, poor, or no fix               |
| NMEA / Logging | Shows raw or parsed GNSS data and supports SD card logging                                         |
| Saved Location | Allows the user to record a location and send it to the backend as an asset                        |
| System Status  | Displays device state, sensors, SD card status, telemetry status, and hardware information         |

---

## GNSS and NMEA Data

The LC29H receiver outputs NMEA sentences. NMEA is a text-based format used by GNSS receivers to report position, time, movement, satellite, and signal information.

The receiver can use multiple satellite constellations depending on configuration and signal availability:

* GPS
* GLONASS
* Galileo
* BeiDou
* QZSS

Common NMEA talker prefixes include:

| Prefix      | Meaning                                    |
| ----------- | ------------------------------------------ |
| `GP`        | GPS                                        |
| `GL`        | GLONASS                                    |
| `GA`        | Galileo                                    |
| `GB` / `BD` | BeiDou                                     |
| `GQ`        | QZSS                                       |
| `GN`        | Combined multi-constellation GNSS solution |

Example:

```text
GNGLL
```

This means a `GLL` sentence generated from a combined multi-constellation GNSS solution.

Common NMEA sentence types used or expected in the project include:

| Sentence | Name                                | Purpose                                                        |
| -------- | ----------------------------------- | -------------------------------------------------------------- |
| `GGA`    | Global Positioning System Fix Data  | Position fix, altitude, satellite count, HDOP, and fix quality |
| `GLL`    | Geographic Position                 | Latitude, longitude, time, and data validity                   |
| `GSA`    | GNSS DOP and Active Satellites      | Active satellites used in the fix and DOP values               |
| `GSV`    | Satellites in View                  | Satellite IDs, elevation, azimuth, and signal strength         |
| `RMC`    | Recommended Minimum GNSS Data       | Time, date, position, speed, course, and validity              |
| `VTG`    | Course Over Ground and Ground Speed | Ground track and speed information                             |
| `ZDA`    | Time and Date                       | UTC time, day, month, year, and local time offset              |
| `GST`    | GNSS Pseudorange Error Statistics   | Estimated position error information                           |
| `GNS`    | GNSS Fix Data                       | Multi-GNSS fix information                                     |
| `TXT`    | Text Message                        | Receiver status or diagnostic messages                         |

The firmware parses these messages to extract practical field data for display, logging, and telemetry.

---

## Signal Quality and Sensor Expansion

G-Cacher maps GNSS reception into simple quality states so that readings are easier to understand during field testing.

| Quality | Typical Meaning                                             |
| ------- | ----------------------------------------------------------- |
| Good    | Stronger fix, more satellites, and lower HDOP               |
| OK      | Usable fix, but lower confidence                            |
| Poor    | Weak signal, fewer satellites, higher HDOP, or unstable fix |
| No Fix  | Position data is not currently reliable                     |

The ESP32-S3 board also includes onboard sensors such as an IMU and BMP280 pressure sensor. These can be used in future versions for sensor fusion, where GNSS, acceleration, rotation, pressure, and altitude data are combined to improve movement estimation and device awareness.

The project also explores Galileo OSNMA where supported. OSNMA can help authenticate Galileo navigation messages and improve resistance to certain spoofing scenarios. It does not prevent RF jamming and should be treated as an additional signal-integrity feature rather than a complete security solution.

---

## Local Data Logging

The device can store GNSS data on an SD card for later analysis.

Logged data may include:

* Latitude
* Longitude
* Altitude
* Date
* Time
* HDOP
* Satellite count
* Fix quality
* Signal quality category
* Saved waypoint or asset information

This allows GNSS points to be reviewed later, imported into the web platform, or used for mapping and terrain visualisation.

---

## Web Platform

The web platform is used to receive, store, visualise, and manage location data from the G-Cacher device.

The ESP32-S3 sends telemetry to the backend over Wi-Fi. The backend is built with FastAPI and provides API endpoints for telemetry, saved locations, tracks, points, and asset management. The frontend provides user-facing tools for viewing and interacting with the collected data.

<p align="center">
  <img width="760" alt="SatCacher home page" src="https://github.com/user-attachments/assets/35294646-b04f-4075-a990-ca09a230cc95" />
</p>

The web platform currently includes four main areas:

| Feature           | Purpose                                                                     |
| ----------------- | --------------------------------------------------------------------------- |
| GeoCache Game     | Uses live or recorded GNSS data to guide a user toward predefined landmarks |
| 3D Terrain Viewer | Visualises latitude, longitude, and altitude points in 3D                   |
| Saved Locations   | Stores and manages important saved locations as assets                      |
| API Documentation | Shows the implemented backend endpoints and request/response models         |

---

## GeoCache Game

The GeoCache Game is designed as a location-based navigation feature for tourists or first-time visitors.

The backend stores predefined landmarks, such as well-known Irish destinations. The G-Cacher device sends its current location to the server. The server compares the user’s movement against the target location and decides whether the user is getting closer or further away.

| Feedback | Meaning                                              |
| -------- | ---------------------------------------------------- |
| Red      | The user is getting closer to the target             |
| Blue     | The user is moving further away from the target      |
| Yellow   | The user has not moved enough for direction feedback |

This creates a GNSS-based hot-and-cold game using real location telemetry.

<p align="center">
  <img width="760" alt="GeoCache game" src="https://github.com/user-attachments/assets/05cf1eac-5227-4aa5-a85f-df75aafeabcc" />
</p>

<p align="center">
  <img width="760" alt="Sample waypoint" src="https://github.com/user-attachments/assets/dd6d2024-0da8-4e26-84df-cb277928729b" />
</p>

---

## 3D Terrain Viewer

The 3D Terrain Viewer displays GNSS points using latitude, longitude, and altitude.

It can be used to:

* Plot recorded survey points
* Visualise a walking path or field test route
* Build a rough 3D model of a mapped area
* Compare real recorded data with simulated points
* View terrain-like data from places such as TUD Tallaght Campus

The viewer can use data from the FastAPI database, imported JSON files, or simulated points. Better results are produced when points are collected across the full area rather than only around the perimeter.

<p align="center">
  <img width="760" alt="3D terrain viewer" src="https://github.com/user-attachments/assets/ae58facb-ceb9-45d1-ad8f-b71f016d5482" />
</p>

<p align="center">
  <img width="760" alt="Sample terrain" src="https://github.com/user-attachments/assets/cc376ff5-f4f1-41e6-b9a4-bbdf83a2dcf4" />
</p>
## TUD Tallaght Campus terrain:
<p align="center">
  <img width="760" alt="TUD Tallaght campus terrain" src="https://github.com/user-attachments/assets/ba3be553-182c-4fd5-b9de-3b4d795bf732" />
</p>

---

## Saved Locations

The Saved Locations feature allows the device to record a location and send it to the backend as an asset.

On the frontend, the saved point can be shown on a Leaflet map and given extra information such as a name, category, and description.

Example asset categories include:

* Parked car
* Pipe
* Cable
* Marker
* Survey point
* Other

This feature is useful for saving important positions that may need to be found again later, such as a parked car, a field marker, or the location of an underground pipe or cable.

<p align="center">
  <img width="760" alt="Saved locations page" src="https://github.com/user-attachments/assets/97315fad-9da7-441b-9735-3e7822f29f4b" />
</p>

<p align="center">
  <img width="760" alt="Database contents" src="https://github.com/user-attachments/assets/8bedb7af-100b-4a19-bb15-da6baa52aa40" />
</p>

---

## FastAPI Backend

The backend is built with FastAPI. It receives telemetry from the ESP32-S3 device, stores records, and provides data to the frontend.

Current backend areas include:

| API Area  | Purpose                                                        |
| --------- | -------------------------------------------------------------- |
| Health    | Checks if the backend is running                               |
| Telemetry | Receives live GNSS data from the ESP32-S3                      |
| Locations | Retrieves saved GNSS locations                                 |
| Latest    | Gets the latest received telemetry point                       |
| Log       | Provides access to telemetry log records                       |
| Points    | Retrieves point data for visualisation                         |
| Track     | Retrieves movement track data                                  |
| Assets    | Creates, updates, retrieves, and deletes saved location assets |

---

## Hardware Required

### 1. Tenstar ESP32-S3

[![Manual](https://img.shields.io/badge/Manual-Blue?style=for-the-badge\&logo=readthedocs\&logoColor=white\&color=1E90FF)](https://manuals.plus/ae/1005006590631744)
[![Purchase](https://img.shields.io/badge/Purchase-Green?style=for-the-badge\&logo=aliexpress\&logoColor=white\&color=2EA44F)](https://www.aliexpress.com/item/1005006454900498.html#nav-description)

### 2. LC29H(AA) GNSS Receiver

[![Manual](https://img.shields.io/badge/Manual-Blue?style=for-the-badge\&logo=readthedocs\&logoColor=white\&color=1E90FF)](https://www.waveshare.com/wiki/LC29H%28XX%29_GPS/RTK_HAT)
[![Purchase](https://img.shields.io/badge/Purchase-Green?style=for-the-badge\&logo=aliexpress\&logoColor=white\&color=2EA44F)](https://www.aliexpress.com/item/1005006000498473.html)

---

## Repository Structure

```text
G-Cacher/
├── firmware/
│   └── esp32s3/
├── web/
│   ├── frontend/
│   └── backend/
├── models/
│   └── enclosure/
├── docs/
├── README.md
└── .gitignore
```

| Folder          | Purpose                                                          |
| --------------- | ---------------------------------------------------------------- |
| `firmware/`     | ESP32-S3 firmware and Arduino sketches                           |
| `web/frontend/` | Frontend website code                                            |
| `web/backend/`  | FastAPI backend code                                             |
| `models/`       | 3D enclosure and box design files                                |
| `docs/`         | Wiring guides, setup notes, API notes, and project documentation |

---

## Learning Portal / Wiki

[![Learn More - GNSS Learning Portal](https://img.shields.io/badge/LEARN%20MORE-GNSS%20Learning%20Portal-39C5BB?style=for-the-badge\&labelColor=00AEEF\&logo=githubpages\&logoColor=white)](https://coolsphere.github.io/GNSS_Satellite/)

The learning portal contains supporting material for GNSS, NMEA, satellite signals, sensors, and setup preparation.

---

## Current Status

The project currently includes:

* ESP32-S3 GNSS device firmware
* LC29H GNSS receiver integration
* ILI9488 touch display interface
* SD card logging support
* Wi-Fi telemetry transfer
* FastAPI backend
* Frontend web dashboard
* GeoCache game prototype
* 3D terrain viewer
* Saved locations and asset mapping
* API documentation page
* 3D printed enclosure prototype

Future improvements may include:

* Improved enclosure design
* More complete field testing
* Improved signal-quality mapping
* Sensor fusion using GNSS, IMU, and barometric readings
* More robust telemetry error handling
* Expanded OSNMA testing where supported

---

## Notes

This repository is part of a student engineering project. The project is intended for learning, prototyping, GNSS experimentation, and field testing. It should not be used as a certified surveying, navigation, or safety-critical positioning system.
