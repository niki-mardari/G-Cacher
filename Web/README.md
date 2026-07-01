# Issues for now 
- Add location saving section
- Make sure docker works on everyones laptop
- Add good documentaion
- Make ping eco on map when showing current location
- Buttons in menu are too small 
- Call the website sommething else like GeoCacher Portal

# SatCache Web Stack

This folder contains the web part of the Tenstar ESP32-S3 GNSS project.

It runs three services with Docker Compose:

- `frontend`: static HTML/CSS/JS dashboard served by Nginx on port `3000`
- `backend`: FastAPI API on port `8000`
- `db`: PostgreSQL database on port `5432`

The ESP32-S3 uploads telemetry to:

```text
http://YOUR_LAPTOP_IP:8000/api/telemetry
```
On smartphone:
```text
http://10.107.7.42:3000/index.html
```
Note: The ip ipv4 address changes so need to check the laptop ip address using:
```text
hostname -i
```

The browser dashboard is available at:

```text
http://localhost:3000
```

The FastAPI docs are available at:

```text
http://localhost:8000/docs
```

## Run locally

From this `Web` folder:

```bash
docker compose up --build
```

Or:

```bash
./scripts/start.sh
```

Stop:

```bash
docker compose down
```

Delete local database data:

```bash
./scripts/reset-db.sh
```

## Test the backend with curl

```bash
curl -X POST "http://127.0.0.1:8000/api/telemetry" \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "tenstar_001",
    "mode": "geocache",
    "lat": 53.349805,
    "lon": -6.260310,
    "alt_m": 40.5,
    "speed_kmph": 2.1,
    "satellites": 35,
    "hdop": 0.8,
    "fix": true,
    "temperature_c": 39.8,
    "pressure_hpa": 1012.4,
    "acc_x": 0.1,
    "acc_y": 0.0,
    "acc_z": 9.8
  }'
```

Then check:

```text
http://localhost:8000/api/latest
http://localhost:8000/api/track
http://localhost:3000
```

## ESP32-S3 upload URL

Find your laptop IP:

```bash
hostname -I
```

Example result:

```text
192.168.1.42
```

The ESP32-S3 Arduino sketch should use:

```cpp
const char *serverUrl = "http://192.168.1.42:8000/api/telemetry";
```

Do not use `127.0.0.1` or `localhost` on the ESP32, because that would mean the ESP32 itself.

## Useful endpoints

| Endpoint | Purpose |
|---|---|
| `GET /` | API info |
| `GET /api/health` | Health check |
| `POST /api/telemetry` | ESP32-S3 upload endpoint |
| `GET /api/latest` | Latest telemetry row |
| `GET /api/log?limit=500` | Full log, newest first |
| `GET /api/track?limit=1000` | Track points, oldest first for map drawing |
| `GET /api/locations` | Geocache locations |
| `POST /api/assets` | Save pipe/object/infrastructure location |
| `GET /api/assets` | View saved assets |

## Folder structure

```text
Web/
├── backend/
│   ├── app/
│   │   └── main.py
│   ├── Dockerfile
│   └── requirements.txt
├── frontend/
│   ├── index.html
│   ├── terrain.html
│   ├── css/
│   ├── js/
│   └── prototypes/
├── database/
│   └── init.sql
├── scripts/
├── docker-compose.yml
├── .env.example
└── README.md
```

## Notes

- The dashboard can simulate telemetry by clicking the map and pressing the simulate button.
- Real ESP32-S3 uploads appear automatically on the map and in the log table.
- `terrain.html` shows a simple 3D route based on `/api/track`.
- The `prototypes/` folder keeps earlier standalone HTML experiments.
