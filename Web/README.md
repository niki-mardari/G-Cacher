# Sat Cacher Web Server

This folder contains the web part of the Sat Cacher project.

It runs three containers:

- **frontend**: the website served by Nginx on port `3000`
- **backend**: the FastAPI API on port `8000`
- **db**: PostgreSQL database on port `5432`

The ESP32-S3 sends GNSS telemetry to the backend. The backend stores it and returns the current geocache command. The website shows the map, route, saved locations, and the 3D viewer.

## Folder layout

```text
Web/
├── backend/
│   ├── app/main.py          # FastAPI backend
│   ├── Dockerfile
│   └── requirements.txt
├── database/
│   └── init.sql             # notes for database setup
├── frontend/
│   ├── index.html           # main Sat Cacher dashboard
│   ├── terrain.html         # new 3D viewer
│   ├── css/
│   ├── js/
│   └── prototypes/          # older experiments and backups
├── scripts/
├── docker-compose.yml
└── README.md
```

## How to run it

From the `Web` folder:

```bash
docker compose up --build
```

If Docker on Linux needs sudo:

```bash
sudo docker compose up --build
```

Run in the background:

```bash
docker compose up --build -d
```

Stop everything:

```bash
docker compose down
```

Reset the database:

```bash
docker compose down -v
docker compose up --build
```

Only use `down -v` when you are okay deleting the local database data.

## Main links

On the same laptop:

```text
Website:       http://localhost:3000
3D viewer:     http://localhost:3000/terrain.html
API docs:      http://localhost:8000/docs
Latest point:  http://localhost:8000/api/latest
Track points:  http://localhost:8000/api/track
Saved spots:   http://localhost:8000/api/assets
```

From a phone on the same network, replace `localhost` with the laptop IP, for example:

```text
http://10.107.7.42:3000
http://10.107.7.42:8000/docs
```

## ESP32-S3 upload URL

In the firmware, the ESP32 should post to:

```text
http://LAPTOP_IP:8000/api/telemetry
```

Example for my laptop:

```text
http://10.107.7.42:8000/api/telemetry
```

Do not use `localhost` in the ESP32 firmware because `localhost` would mean the ESP32 itself.

## What the backend does

The backend creates the database tables on startup if they do not already exist.

It stores:

- telemetry points from the ESP32
- geocache landmark locations
- saved marked locations/assets

The geocache destinations include famous Irish landmarks and TU Dublin Tallaght. When telemetry arrives, the backend checks the closest active landmark and sends a command back to the ESP32.

## Useful API endpoints

```text
GET     /api/health
GET     /api/locations
POST    /api/telemetry
GET     /api/latest
GET     /api/track?limit=1000
GET     /api/points?limit=1000
GET     /api/log?limit=500
GET     /api/assets
POST    /api/assets
PATCH   /api/assets/{asset_id}
DELETE  /api/assets/{asset_id}
DELETE  /api/dev/clear-telemetry
```

## Testing without the ESP32

Click on the map in the dashboard, then press **Send simulated telemetry**.

Or use curl:

```bash
curl -X POST "http://localhost:8000/api/telemetry" \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "browser_test",
    "mode": "geocache",
    "lat": 53.2911,
    "lon": -6.3630,
    "alt_m": 100.0,
    "speed_kmph": 1.0,
    "satellites": 35,
    "hdop": 0.8,
    "fix": true,
    "temperature_c": 31.0,
    "pressure_hpa": 1012.0,
    "acc_x": 0.1,
    "acc_y": 0.0,
    "acc_z": 9.8
  }'
```

Then open:

```text
http://localhost:8000/api/latest
http://localhost:3000
```

## Saved locations

The ESP32 firmware can save a marked location by posting to `/api/assets`.

The dashboard also has a **Save latest GNSS point** button. After saving, locations appear on the Leaflet map and can be renamed/tagged in the Saved Locations section.

Example uses:

- car parked location
- pipe or valve in the ground
- construction marker
- survey point
- cable route marker

## 3D viewer

The new `3D_Modelling_Test_Sample.html` prototype has replaced `frontend/terrain.html`.

The old `terrain.html` was moved to:

```text
frontend/prototypes/terrain-original.html
```

The new 3D viewer automatically uses the current host name, so it should work from either the laptop or another device on the same network.
