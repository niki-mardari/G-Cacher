# G-Cacher

**A portable GNSS surveying, logging, and location-mapping tool built around an ESP32-S3, LC29H GNSS receiver, touch display, local storage, and a FastAPI web platform.**

<p align="center">
  <img width="942" height="1071" alt="G-Cacher device render" src="https://github.com/user-attachments/assets/1a463576-31dd-4d96-840d-1e0fd5fb8bca" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/GNSS-Surveying-1E90FF?style=for-the-badge" />
  <img src="https://img.shields.io/badge/ESP32--S3-Firmware-404040?style=for-the-badge&logo=espressif&logoColor=white" />
  <img src="https://img.shields.io/badge/FastAPI-Backend-009688?style=for-the-badge&logo=fastapi&logoColor=white" />
  <img src="https://img.shields.io/badge/Python-API-3776AB?style=for-the-badge&logo=python&logoColor=white" />
  <img src="https://img.shields.io/badge/JavaScript-Frontend-F7DF1E?style=for-the-badge&logo=javascript&logoColor=black" />
  <img src="https://img.shields.io/badge/Arduino-Firmware-00979D?style=for-the-badge&logo=arduino&logoColor=white" />
</p>

---

## Overview

G-Cacher is an engineering student maker project developed for the BIP26 programme. The project combines embedded hardware, GNSS satellite positioning, local data logging, wireless telemetry, backend storage, and a browser-based frontend.

The physical device reads raw NMEA data from an LC29H GNSS receiver, parses useful positioning and signal-quality information, displays it on a touch interface, saves selected records to an SD card, and can transmit telemetry to a web server over Wi-Fi.

The web platform receives GNSS telemetry from the device, stores useful location records, visualises points in different ways, and provides a FastAPI-based interface for the frontend and device communication.

---

## Project Goals

G-Cacher is designed to explore how a low-cost embedded system can be used for practical GNSS-based surveying and location workflows.

The main goals are:

* Read and parse live GNSS data from multiple satellite constellations
* Display useful position and signal information on a portable device
* Save GNSS records locally using SD card storage
* Send telemetry from the ESP32-S3 device to a backend server
* Store and retrieve location data through a FastAPI backend
* Provide frontend tools for geocaching, saved locations, API documentation, and 3D terrain visualisation
* Build a portable enclosure suitable for demonstration and field testing

---

## System Architecture

```text
GNSS Satellites
      |
      v
LC29H GNSS Receiver
      |
      | UART / NMEA
      v
ESP32-S3 Device
      |
      | Touch UI, SD Logging, Sensors
      |
      | Wi-Fi / Mobile Hotspot
      v
FastAPI Backend
      |
      | API + Database / Stored Records
      v
Frontend Web Dashboard
```

---

## Physical Device

The G-Cacher hardware is built around the Tenstar ESP32-S3 development board and an LC29H GNSS receiver.

The device includes:

| Component                                  | Purpose                                                                |
| ------------------------------------------ | ---------------------------------------------------------------------- |
| Tenstar ESP32-S3                           | Main microcontroller, Wi-Fi telemetry, display control, firmware logic |
| LC29H(AA) GNSS receiver                    | Receives satellite positioning data and outputs NMEA strings           |
| ILI9488 touch display                      | Displays menus, live GNSS data, signal quality, and device status      |
| SD card slot                               | Stores GNSS records locally for later analysis                         |
| BMP280 sensor                              | Measures pressure and temperature                                      |
| QMI8658C IMU                               | Measures acceleration and rotation                                     |
| 2000 mAh 3.7 V rechargeable Li-ion battery | Portable power source for field use                                    |
| 3D printed enclosure                       | Protects and holds the device hardware                                 |

Battery life depends on display brightness, Wi-Fi usage, SD logging, GNSS activity, and firmware behaviour. The current battery target is approximately up to 5 hours under heavy use.

---

## Device Interface

The firmware uses a touch-based menu system on the ILI9488 display.

The current device interface is designed around pages such as:

| Menu Page      | Purpose                                                                                                 |
| -------------- | ------------------------------------------------------------------------------------------------------- |
| Live GNSS      | Shows parsed latitude, longitude, altitude, speed, HDOP, satellite count, date, and time                |
| Signal Quality | Displays GNSS quality indicators such as fix status, HDOP, satellite count, and signal strength mapping |
| NMEA / Logging | Shows raw or parsed GNSS data and supports local SD logging                                             |
| Saved Location | Allows a location to be recorded and sent to the backend as a saved asset                               |
| System Status  | Displays hardware status, sensor readings, storage state, and telemetry state                           |

---

## GNSS and NMEA Data

The LC29H receiver outputs NMEA sentences. NMEA is a standard text-based format used by GNSS receivers to report position, time, satellite, and movement data.

The receiver can use satellite constellations such as:

* GPS
* GLONASS
* Galileo
* BeiDou
* QZSS, depending on receiver configuration and signal availability

Common NMEA talker prefixes include:

| Prefix      | Meaning                                              |
| ----------- | ---------------------------------------------------- |
| `GP`        | GPS                                                  |
| `GL`        | GLONASS                                              |
| `GA`        | Galileo                                              |
| `GB` / `BD` | BeiDou                                               |
| `GQ`        | QZSS                                                 |
| `GN`        | Combined GNSS solution using multiple constellations |

Example:

```text
GNGLL
```

This means a GLL sentence generated from a combined multi-constellation GNSS solution.

Common NMEA sentence types used or expected in the project include:

| Sentence | Name                                | Purpose                                                        |
| -------- | ----------------------------------- | -------------------------------------------------------------- |
| `GGA`    | Global Positioning System Fix Data  | Position fix, altitude, satellite count, HDOP, and fix quality |
| `GLL`    | Geographic Position                 | Latitude, longitude, time, and data validity                   |
| `GSA`    | GNSS DOP and Active Satellites      | Active satellites used in the fix and DOP values               |
| `GSV`    | Satellites in View                  | Satellite IDs, elevation, azimuth, and signal strength         |
| `RMC`    | Recommended Minimum GNSS Data       | Time, date, position, speed, course, and validity              |
| `VTG`    | Course Over Ground and Ground Speed | Ground track and speed information                             |
| `ZDA`    | Time and Date                       | UTC time, day, month, year, and local time zone offset         |
| `GST`    | GNSS Pseudorange Error Statistics   | Estimated position error information                           |
| `GNS`    | GNSS Fix Data                       | Multi-GNSS fix information                                     |
| `TXT`    | Text Message                        | Receiver status or diagnostic messages                         |

The firmware parses these strings to extract useful fields such as:

* Latitude
* Longitude
* Altitude
* Ground speed
* Date and time
* HDOP
* Satellite count
* Fix quality
* Signal quality indicators

---

## Signal Quality Mapping

The project maps GNSS signal quality into simple categories that are easier to understand during field testing.

Example categories:

| Quality | Typical Meaning                                              |
| ------- | ------------------------------------------------------------ |
| Good    | Stronger fix, more satellites, lower HDOP                    |
| OK      | Usable fix but lower confidence                              |
| Poor    | Weak signal, low satellite count, high HDOP, or unstable fix |
| No Fix  | Position is not currently reliable                           |

This makes the device easier to use in real field conditions where raw satellite values may not be immediately meaningful.

---

## OSNMA and Signal Integrity

The project explores Galileo OSNMA support where available.

OSNMA stands for Open Service Navigation Message Authentication. It is used by Galileo to authenticate navigation message data. In practical terms, this can help receivers detect certain forms of navigation-message spoofing.

OSNMA does not make the device immune to all GNSS attacks. It does not prevent RF jamming, and it depends on receiver support, satellite visibility, implementation, and correct configuration.

For this project, OSNMA is treated as an additional signal-integrity feature rather than a complete security solution.

---

## Local Data Logging

The device includes SD card logging for recording GNSS data during field tests.

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
* Saved asset or waypoint information

This allows collected GNSS data to be reviewed later, imported into the frontend, or used for mapping and terrain visualisation.

---

## Web Platform

The web platform provides a frontend dashboard and a FastAPI backend.

The ESP32-S3 connects to a mobile hotspot and sends telemetry to the backend server. The backend stores location records and exposes API endpoints used by the frontend.

The web application currently includes four main areas:

| Feature           | Purpose                                                                    |
| ----------------- | -------------------------------------------------------------------------- |
| GeoCache Game     | Uses live device position to guide users toward predefined Irish landmarks |
| 3D Terrain Viewer | Visualises latitude, longitude, and altitude points in 3D                  |
| Saved Locations   | Stores and labels saved device locations as assets                         |
| API Documentation | Shows implemented FastAPI endpoints and request/response models            |

---

## GeoCache Game

The GeoCache Game is designed as a tourist-friendly navigation feature.

The server stores predefined landmark locations, such as well-known Irish destinations. The G-Cacher device sends its current GNSS position to the backend, and the backend calculates which landmark is closest or which target the user is navigating toward.

The system compares the current position with previous movement to decide whether the user is getting closer or further away.

Example feedback logic:

| Device Feedback | Meaning                                              |
| --------------- | ---------------------------------------------------- |
| Red             | The user is getting closer to the target             |
| Blue            | The user is moving further away from the target      |
| Yellow          | The user has not moved enough for direction feedback |

This creates a hot-and-cold navigation game using real GNSS telemetry.

---

## 3D Terrain Viewer

The 3D Terrain Viewer visualises GNSS points using latitude, longitude, and altitude.

It can be used to:

* Plot recorded GNSS survey points
* Visualise a perimeter or walking path
* Generate a rough 3D view of a mapped area
* Compare real recorded points with simulated data
* View locations such as a campus, field, or test route

The viewer can use data from:

* FastAPI backend records
* Imported JSON files
* Simulated point data

For better terrain results, more points should be collected across the area instead of only recording the perimeter.

---

## Saved Locations

The Saved Locations feature allows the device to record a location and send it to the backend as an asset.

On the frontend, the user can view the saved location on a Leaflet map and add useful metadata.

Example saved asset categories:

* Parked car
* Pipe
* Cable
* Marker
* Survey point
* Other

A saved location can also include:

* Name
* Tag
* Description
* Latitude
* Longitude
* Date and time recorded

This feature is useful for remembering important places or marking objects that may need to be found again later.

Example use cases:

* Saving where a car is parked
* Marking a buried pipe or cable
* Recording a survey point
* Saving a field test location
* Returning to a point after vegetation or site conditions have changed

---

## FastAPI Backend

The backend is built with FastAPI and provides API endpoints for receiving telemetry, storing location data, retrieving records, and supporting frontend features.

The API documentation is available through the FastAPI Swagger interface.

Current backend areas include:

| API Area  | Purpose                                                    |
| --------- | ---------------------------------------------------------- |
| Health    | Check if the backend is running                            |
| Telemetry | Receive live GNSS data from the ESP32-S3                   |
| Locations | Retrieve saved GNSS locations                              |
| Latest    | Get the latest received telemetry point                    |
| Log       | View logged telemetry records                              |
| Points    | Retrieve point data for visualisation                      |
| Track     | Retrieve movement track data                               |
| Assets    | Create, update, retrieve, and delete saved location assets |

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

A clean project structure is recommended so that firmware, web code, models, and documentation do not become mixed together.

```text
G-Cacher/
├── firmware/
│   └── esp32s3/
│       └── device_firmware/
├── web/
│   ├── frontend/
│   └── backend/
├── models/
│   └── enclosure/
├── docs/
├── README.md
└── .gitignore
```

Suggested folder responsibilities:

| Folder          | Purpose                                                              |
| --------------- | -------------------------------------------------------------------- |
| `firmware/`     | ESP32-S3 firmware and Arduino sketches                               |
| `web/frontend/` | Frontend website code                                                |
| `web/backend/`  | FastAPI backend code                                                 |
| `models/`       | 3D enclosure and box design files                                    |
| `docs/`         | Technical notes, wiring guides, API notes, and project documentation |

---

## Learning Portal / Wiki

[![Learn More - GNSS Learning Portal](https://img.shields.io/badge/LEARN%20MORE-GNSS%20Learning%20Portal-39C5BB?style=for-the-badge\&labelColor=00AEEF\&logo=githubpages\&logoColor=white)](https://coolsphere.github.io/GNSS_Satellite/)

The learning portal contains supporting material for GNSS, NMEA, satellite signals, sensors, and setup preparation.

---

## Members / Contributors

* Andrei-Eduard Rusu
* Carla Daniela Zegarra Bernabe
* Niki Mardari
* Szymon Blazejowski

---

## Project Status

The project currently includes:

* Working ESP32-S3 GNSS device firmware
* LC29H GNSS receiver integration
* Touch display interface
* SD card logging support
* Wi-Fi telemetry transfer
* FastAPI backend
* Frontend web dashboard
* GeoCache game prototype
* 3D terrain viewer
* Saved locations and asset mapping
* API documentation page
* Physical device enclosure prototype

Future work may include:

* Improved enclosure design
* More complete field testing
* Improved signal-quality mapping
* Better frontend styling and workflow
* Sensor fusion using GNSS, IMU, and barometric readings
* More robust telemetry error handling
* Expanded OSNMA testing where supported

---

## Gallery

### SatCacher Home Page

<img width="1276" height="710" alt="SatCacher home page" src="https://github.com/user-attachments/assets/35294646-b04f-4075-a990-ca09a230cc95" />

### GeoCache Game

<img width="1276" height="717" alt="GeoCache game" src="https://github.com/user-attachments/assets/05cf1eac-5227-4aa5-a85f-df75aafeabcc" />

### Sample Waypoint

<img width="1276" height="711" alt="Sample waypoint" src="https://github.com/user-attachments/assets/dd6d2024-0da8-4e26-84df-cb277928729b" />

### 3D Terrain Viewer

<img width="1269" height="739" alt="3D terrain viewer" src="https://github.com/user-attachments/assets/ae58facb-ceb9-45d1-ad8f-b71f016d5482" />

### Sample Terrain

<img width="1273" height="714" alt="Sample terrain" src="https://github.com/user-attachments/assets/cc376ff5-f4f1-41e6-b9a4-bbdf83a2dcf4" />

### TUD Tallaght Campus Terrain

<img width="1274" height="714" alt="TUD Tallaght campus terrain" src="https://github.com/user-attachments/assets/ba3be553-182c-4fd5-b9de-3b4d795bf732" />

### Saved Locations

<img width="1276" height="717" alt="Saved locations page" src="https://github.com/user-attachments/assets/97315fad-9da7-441b-9735-3e7822f29f4b" />

### Database Contents

<img width="1278" height="715" alt="Database contents" src="https://github.com/user-attachments/assets/8bedb7af-100b-4a19-bb15-da6baa52aa40" />

### API Documentation

<img width="1278" height="715" alt="SatCacher GNSS API documentation" src="https://github.com/user-attachments/assets/8bedb7af-100b-4a19-bb15-da6baa52aa40" />

### G-Cacher Device

<img width="949" height="1041" alt="G-Cacher physical device" src="https://github.com/user-attachments/assets/d02fb363-c22b-4e05-8810-f08625705583" />

---

## Notes

This repository is part of a student engineering project. The project is intended for learning, prototyping, GNSS experimentation, and field testing. It should not be used as a certified surveying, navigation, or safety-critical positioning system.
