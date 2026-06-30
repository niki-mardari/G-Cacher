from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import Optional
from datetime import datetime, timezone
import sqlite3
import math


DB_PATH = "gnss_project.db"


app = FastAPI(
    title="Tenstar ESP32-S3 GNSS Server",
    description="Receives GNSS telemetry and sends commands back to the ESP32-S3.",
    version="0.1.0",
)


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# -------------------------------------------------------------------
# Example geocache locations
# Later these should move into the database.
# -------------------------------------------------------------------

GEOCACHE_LOCATIONS = [
    {
        "id": 1,
        "name": "General Post Office, Dublin",
        "lat": 53.349805,
        "lon": -6.260310,
        "radius_m": 25,
        "clue": "Find the historic building connected to the 1916 Rising.",
        "hint": "You should be near O'Connell Street.",
    },
    {
        "id": 2,
        "name": "Dublin Castle",
        "lat": 53.342886,
        "lon": -6.267428,
        "radius_m": 25,
        "clue": "Go to the old seat of power in Dublin.",
        "hint": "You are looking for a castle in the city centre.",
    },
    {
        "id": 3,
        "name": "Trinity College Dublin",
        "lat": 53.343793,
        "lon": -6.254572,
        "radius_m": 25,
        "clue": "Find the famous university with the Book of Kells.",
        "hint": "You should be near College Green.",
    },
]


# Stores previous distance for each device while the server is running.
# Later this should move into the database.
previous_distance_by_device = {}


# -------------------------------------------------------------------
# Data models
# -------------------------------------------------------------------

class TelemetryIn(BaseModel):
    device_id: str
    mode: str = "geocache"

    lat: Optional[float] = None
    lon: Optional[float] = None
    alt_m: Optional[float] = None
    speed_kmph: Optional[float] = None

    satellites: Optional[int] = None
    hdop: Optional[float] = None
    fix: bool = False

    temperature_c: Optional[float] = None
    pressure_hpa: Optional[float] = None

    acc_x: Optional[float] = None
    acc_y: Optional[float] = None
    acc_z: Optional[float] = None


class CommandOut(BaseModel):
    ok: bool
    command: str
    screen_color: str
    title: str
    message: str

    target_name: Optional[str] = None
    target_lat: Optional[float] = None
    target_lon: Optional[float] = None
    distance_m: Optional[float] = None

    beep: bool = False


# -------------------------------------------------------------------
# Database
# -------------------------------------------------------------------

def get_db_connection():
    conn = sqlite3.connect(DB_PATH)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    conn = get_db_connection()
    cur = conn.cursor()

    cur.execute(
        """
        CREATE TABLE IF NOT EXISTS telemetry_points (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            created_at TEXT NOT NULL,

            device_id TEXT NOT NULL,
            mode TEXT NOT NULL,

            lat REAL,
            lon REAL,
            alt_m REAL,
            speed_kmph REAL,

            satellites INTEGER,
            hdop REAL,
            fix INTEGER,

            temperature_c REAL,
            pressure_hpa REAL,

            acc_x REAL,
            acc_y REAL,
            acc_z REAL,

            command TEXT,
            screen_color TEXT,
            message TEXT,
            target_name TEXT,
            distance_m REAL
        )
        """
    )

    conn.commit()
    conn.close()


init_db()


# -------------------------------------------------------------------
# Helper functions
# -------------------------------------------------------------------

def haversine_distance_m(lat1, lon1, lat2, lon2):
    """
    Calculates distance between two latitude/longitude points in metres.
    Good enough for the first version of the project.
    """

    radius_earth_m = 6371000

    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)

    delta_phi = math.radians(lat2 - lat1)
    delta_lambda = math.radians(lon2 - lon1)

    a = (
        math.sin(delta_phi / 2) ** 2
        + math.cos(phi1) * math.cos(phi2) * math.sin(delta_lambda / 2) ** 2
    )

    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))

    return radius_earth_m * c


def get_nearest_location(lat, lon):
    nearest = None
    nearest_distance = None

    for location in GEOCACHE_LOCATIONS:
        distance_m = haversine_distance_m(
            lat,
            lon,
            location["lat"],
            location["lon"],
        )

        if nearest is None or distance_m < nearest_distance:
            nearest = location
            nearest_distance = distance_m

    return nearest, nearest_distance


def build_command(data: TelemetryIn):
    if not data.fix or data.lat is None or data.lon is None:
        return {
            "ok": True,
            "command": "GNSS_WARNING",
            "screen_color": "orange",
            "title": "No GNSS Fix",
            "message": "Move outside or wait for a better satellite fix.",
            "target_name": None,
            "target_lat": None,
            "target_lon": None,
            "distance_m": None,
            "beep": False,
        }

    if data.satellites is not None and data.satellites < 6:
        return {
            "ok": True,
            "command": "GNSS_WARNING",
            "screen_color": "orange",
            "title": "Weak GNSS",
            "message": "Satellite count is low. Move to open sky.",
            "target_name": None,
            "target_lat": None,
            "target_lon": None,
            "distance_m": None,
            "beep": False,
        }

    if data.hdop is not None and data.hdop > 5:
        return {
            "ok": True,
            "command": "GNSS_WARNING",
            "screen_color": "orange",
            "title": "Poor Accuracy",
            "message": "HDOP is high. Wait for a better fix.",
            "target_name": None,
            "target_lat": None,
            "target_lon": None,
            "distance_m": None,
            "beep": False,
        }

    target, distance_m = get_nearest_location(data.lat, data.lon)

    previous_distance = previous_distance_by_device.get(data.device_id)
    previous_distance_by_device[data.device_id] = distance_m

    arrival_radius = target["radius_m"]

    if distance_m <= arrival_radius:
        return {
            "ok": True,
            "command": "TARGET_REACHED",
            "screen_color": "green",
            "title": "Target Reached",
            "message": "You reached the location. Next clue unlocked.",
            "target_name": target["name"],
            "target_lat": target["lat"],
            "target_lon": target["lon"],
            "distance_m": round(distance_m, 1),
            "beep": True,
        }

    if previous_distance is None:
        return {
            "ok": True,
            "command": "TARGET_ASSIGNED",
            "screen_color": "yellow",
            "title": target["name"],
            "message": target["clue"],
            "target_name": target["name"],
            "target_lat": target["lat"],
            "target_lon": target["lon"],
            "distance_m": round(distance_m, 1),
            "beep": False,
        }

    # Tolerance prevents flickering caused by normal GNSS jitter.
    tolerance_m = 5.0

    if distance_m < previous_distance - tolerance_m:
        return {
            "ok": True,
            "command": "GETTING_CLOSER",
            "screen_color": "red",
            "title": "Getting Closer",
            "message": f"Good. Keep going. Distance: {distance_m:.1f} m",
            "target_name": target["name"],
            "target_lat": target["lat"],
            "target_lon": target["lon"],
            "distance_m": round(distance_m, 1),
            "beep": False,
        }

    if distance_m > previous_distance + tolerance_m:
        return {
            "ok": True,
            "command": "GETTING_FURTHER",
            "screen_color": "blue",
            "title": "Getting Further",
            "message": target["hint"],
            "target_name": target["name"],
            "target_lat": target["lat"],
            "target_lon": target["lon"],
            "distance_m": round(distance_m, 1),
            "beep": False,
        }

    return {
        "ok": True,
        "command": "KEEP_GOING",
        "screen_color": "yellow",
        "title": "Keep Moving",
        "message": f"Distance: {distance_m:.1f} m. Movement change is small.",
        "target_name": target["name"],
        "target_lat": target["lat"],
        "target_lon": target["lon"],
        "distance_m": round(distance_m, 1),
        "beep": False,
    }


def save_telemetry(data: TelemetryIn, command: dict):
    conn = get_db_connection()
    cur = conn.cursor()

    cur.execute(
        """
        INSERT INTO telemetry_points (
            created_at,
            device_id,
            mode,
            lat,
            lon,
            alt_m,
            speed_kmph,
            satellites,
            hdop,
            fix,
            temperature_c,
            pressure_hpa,
            acc_x,
            acc_y,
            acc_z,
            command,
            screen_color,
            message,
            target_name,
            distance_m
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """,
        (
            datetime.now(timezone.utc).isoformat(),
            data.device_id,
            data.mode,
            data.lat,
            data.lon,
            data.alt_m,
            data.speed_kmph,
            data.satellites,
            data.hdop,
            1 if data.fix else 0,
            data.temperature_c,
            data.pressure_hpa,
            data.acc_x,
            data.acc_y,
            data.acc_z,
            command["command"],
            command["screen_color"],
            command["message"],
            command["target_name"],
            command["distance_m"],
        ),
    )

    conn.commit()
    conn.close()


# -------------------------------------------------------------------
# API routes
# -------------------------------------------------------------------

@app.get("/")
def root():
    return {
        "status": "ok",
        "message": "Tenstar ESP32-S3 GNSS API is running.",
        "docs": "/docs",
    }


@app.get("/api/locations")
def get_locations():
    return {
        "locations": GEOCACHE_LOCATIONS
    }


@app.post("/api/telemetry", response_model=CommandOut)
def receive_telemetry(data: TelemetryIn):
    command = build_command(data)
    save_telemetry(data, command)
    return command


@app.get("/api/latest")
def get_latest():
    conn = get_db_connection()
    cur = conn.cursor()

    cur.execute(
        """
        SELECT *
        FROM telemetry_points
        ORDER BY id DESC
        LIMIT 1
        """
    )

    row = cur.fetchone()
    conn.close()

    if row is None:
        return {
            "ok": False,
            "message": "No telemetry received yet."
        }

    return dict(row)


@app.get("/api/points")
def get_points(limit: int = 100):
    conn = get_db_connection()
    cur = conn.cursor()

    cur.execute(
        """
        SELECT *
        FROM telemetry_points
        ORDER BY id DESC
        LIMIT ?
        """,
        (limit,),
    )

    rows = cur.fetchall()
    conn.close()

    return {
        "points": [dict(row) for row in rows]
    }