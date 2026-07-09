from __future__ import annotations

import math
import os
import time
from datetime import datetime, timezone
from typing import Optional

import psycopg2
import psycopg2.extras
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel, Field

DATABASE_URL = os.getenv(
    "DATABASE_URL",
    "postgresql://satcache_user:satcache_password@db:5432/satcache_db",
)

DISTANCE_JITTER_TOLERANCE_M = 5.0

app = FastAPI(
    title="SatCacher GNSS API",
    description="Backend for ESP32-S3 GNSS telemetry, the SatCacher geocache game, 3D route data, and saved field locations.",
    version="0.2.0",
)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=False,
    allow_methods=["*"],
    allow_headers=["*"],
)

previous_distance_by_device: dict[str, float] = {}


class TelemetryIn(BaseModel):
    device_id: str = Field(..., examples=["tenstar_001"])
    mode: str = Field("geocache", examples=["geocache"])
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


class AssetIn(BaseModel):
    device_id: str = "tenstar_001"
    asset_type: str = Field("marker", examples=["car", "pipe", "cable", "survey", "construction"])
    name: str = "Unnamed saved location"
    lat: float
    lon: float
    alt_m: Optional[float] = None
    description: Optional[str] = None
    satellites: Optional[int] = None
    hdop: Optional[float] = None


class AssetUpdate(BaseModel):
    asset_type: Optional[str] = None
    name: Optional[str] = None
    description: Optional[str] = None


def db_connect(retries: int = 20, delay_s: float = 1.0):
    last_error = None
    for _ in range(retries):
        try:
            return psycopg2.connect(DATABASE_URL)
        except psycopg2.OperationalError as exc:
            last_error = exc
            time.sleep(delay_s)
    raise RuntimeError(f"Could not connect to database: {last_error}")


def fetch_all(query: str, params: tuple = ()): 
    with db_connect() as conn:
        with conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor) as cur:
            cur.execute(query, params)
            return [dict(row) for row in cur.fetchall()]


def fetch_one(query: str, params: tuple = ()): 
    with db_connect() as conn:
        with conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor) as cur:
            cur.execute(query, params)
            row = cur.fetchone()
            return dict(row) if row else None


def execute_one(query: str, params: tuple = ()): 
    with db_connect() as conn:
        with conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor) as cur:
            cur.execute(query, params)
            row = cur.fetchone()
            conn.commit()
            return dict(row) if row else None


def execute(query: str, params: tuple = ()): 
    with db_connect() as conn:
        with conn.cursor() as cur:
            cur.execute(query, params)
            conn.commit()


def init_db():
    with db_connect() as conn:
        with conn.cursor() as cur:
            cur.execute(
                """
                CREATE TABLE IF NOT EXISTS game_locations (
                    id SERIAL PRIMARY KEY,
                    name TEXT NOT NULL,
                    hint TEXT NOT NULL,
                    lat DOUBLE PRECISION NOT NULL,
                    lon DOUBLE PRECISION NOT NULL,
                    radius_m DOUBLE PRECISION NOT NULL DEFAULT 30,
                    play_order INTEGER NOT NULL DEFAULT 0,
                    active BOOLEAN NOT NULL DEFAULT TRUE
                );
                """
            )
            cur.execute(
                """
                CREATE TABLE IF NOT EXISTS telemetry_points (
                    id SERIAL PRIMARY KEY,
                    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
                    device_id TEXT NOT NULL,
                    mode TEXT NOT NULL,
                    lat DOUBLE PRECISION,
                    lon DOUBLE PRECISION,
                    alt_m DOUBLE PRECISION,
                    speed_kmph DOUBLE PRECISION,
                    satellites INTEGER,
                    hdop DOUBLE PRECISION,
                    fix BOOLEAN,
                    temperature_c DOUBLE PRECISION,
                    pressure_hpa DOUBLE PRECISION,
                    acc_x DOUBLE PRECISION,
                    acc_y DOUBLE PRECISION,
                    acc_z DOUBLE PRECISION,
                    command TEXT,
                    screen_color TEXT,
                    message TEXT,
                    target_name TEXT,
                    distance_m DOUBLE PRECISION
                );
                """
            )
            cur.execute(
                """
                CREATE TABLE IF NOT EXISTS saved_assets (
                    id SERIAL PRIMARY KEY,
                    created_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
                    device_id TEXT NOT NULL,
                    asset_type TEXT NOT NULL,
                    name TEXT NOT NULL,
                    lat DOUBLE PRECISION NOT NULL,
                    lon DOUBLE PRECISION NOT NULL,
                    alt_m DOUBLE PRECISION,
                    description TEXT,
                    satellites INTEGER,
                    hdop DOUBLE PRECISION
                );
                """
            )

            seed_locations = [
            ("TU Dublin Tallaght", "Local SatCacher testing point near the Tallaght campus.", 53.2911, -6.3630, 35, 1),
            ("Trinity College Dublin", "Ireland's oldest university and home to the Book of Kells.", 53.3438, -6.2546, 30, 2),
            ("Dublin Castle", "Historic castle and former centre of British administration in Ireland.", 53.3429, -6.2674, 30, 3),
            ("St Patrick's Cathedral", "Large national cathedral in Dublin linked with Jonathan Swift.", 53.3395, -6.2716, 30, 4),
            ("The Spire", "Tall stainless steel monument on O'Connell Street in Dublin.", 53.3498, -6.2603, 30, 5),
            ("Newgrange", "Prehistoric passage tomb in County Meath.", 53.6947, -6.4755, 60, 6),
            ("Rock of Cashel", "Historic limestone site in County Tipperary.", 52.5200, -7.8900, 60, 7),
            ("Cliffs of Moher", "Famous Atlantic sea cliffs in County Clare.", 52.9715, -9.4309, 80, 8),
            ("TUD Blanchardstown", "Best University in Blanchardstown.", 53.4055, -6.3786, 35, 9),
            ]
            
            for name, hint, lat, lon, radius_m, order in seed_locations:
                cur.execute("SELECT id FROM game_locations WHERE name = %s LIMIT 1;", (name,))
                if cur.fetchone() is None:
                    cur.execute(
                        """
                        INSERT INTO game_locations (name, hint, lat, lon, radius_m, play_order, active)
                        VALUES (%s, %s, %s, %s, %s, %s, TRUE);
                        """,
                        (name, hint, lat, lon, radius_m, order),
                    )
        conn.commit()


@app.on_event("startup")
def startup_event():
    init_db()


def haversine_distance_m(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    radius_earth_m = 6_371_000.0
    phi1 = math.radians(lat1)
    phi2 = math.radians(lat2)
    delta_phi = math.radians(lat2 - lat1)
    delta_lambda = math.radians(lon2 - lon1)
    a = math.sin(delta_phi / 2) ** 2 + math.cos(phi1) * math.cos(phi2) * math.sin(delta_lambda / 2) ** 2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return radius_earth_m * c


def get_nearest_location(lat: float, lon: float):
    locations = fetch_all(
        """
        SELECT id, name, hint, lat, lon, radius_m, play_order
        FROM game_locations
        WHERE active = TRUE
        ORDER BY play_order ASC;
        """
    )
    if not locations:
        raise HTTPException(status_code=500, detail="No geocache locations configured")
    nearest = None
    nearest_distance = None
    for location in locations:
        d = haversine_distance_m(lat, lon, location["lat"], location["lon"])
        if nearest is None or d < nearest_distance:
            nearest = location
            nearest_distance = d
    return nearest, nearest_distance


def build_command(data: TelemetryIn) -> dict:
    if not data.fix or data.lat is None or data.lon is None:
        return {"ok": True, "command": "GNSS_WARNING", "screen_color": "orange", "title": "No GNSS Fix", "message": "Move outside or wait for a better satellite fix.", "target_name": None, "target_lat": None, "target_lon": None, "distance_m": None, "beep": False}
    if data.satellites is not None and data.satellites < 6:
        return {"ok": True, "command": "GNSS_WARNING", "screen_color": "orange", "title": "Weak GNSS", "message": "Satellite count is low. Move to open sky.", "target_name": None, "target_lat": None, "target_lon": None, "distance_m": None, "beep": False}
    if data.hdop is not None and data.hdop > 5:
        return {"ok": True, "command": "GNSS_WARNING", "screen_color": "orange", "title": "Poor Accuracy", "message": "HDOP is high. Wait for a better fix.", "target_name": None, "target_lat": None, "target_lon": None, "distance_m": None, "beep": False}

    target, distance_m = get_nearest_location(data.lat, data.lon)
    previous_distance = previous_distance_by_device.get(data.device_id)
    previous_distance_by_device[data.device_id] = distance_m

    common = {
        "target_name": target["name"],
        "target_lat": target["lat"],
        "target_lon": target["lon"],
        "distance_m": round(distance_m, 1),
    }
    if distance_m <= float(target["radius_m"]):
        return {"ok": True, "command": "TARGET_REACHED", "screen_color": "green", "title": "Target Reached", "message": f"You reached {target['name']}.", "beep": True, **common}
    if previous_distance is None:
        return {"ok": True, "command": "TARGET_ASSIGNED", "screen_color": "yellow", "title": target["name"], "message": target["hint"], "beep": False, **common}
    if distance_m < previous_distance - DISTANCE_JITTER_TOLERANCE_M:
        return {"ok": True, "command": "GETTING_CLOSER", "screen_color": "red", "title": "Getting Closer", "message": f"Distance: {distance_m:.1f} m", "beep": False, **common}
    if distance_m > previous_distance + DISTANCE_JITTER_TOLERANCE_M:
        return {"ok": True, "command": "GETTING_FURTHER", "screen_color": "blue", "title": "Getting Further", "message": target["hint"], "beep": False, **common}
    return {"ok": True, "command": "KEEP_GOING", "screen_color": "yellow", "title": "Keep Moving", "message": f"Distance: {distance_m:.1f} m. Movement change is small.", "beep": False, **common}


def save_telemetry(data: TelemetryIn, command: dict) -> dict:
    return execute_one(
        """
        INSERT INTO telemetry_points (
            device_id, mode, lat, lon, alt_m, speed_kmph, satellites, hdop, fix,
            temperature_c, pressure_hpa, acc_x, acc_y, acc_z,
            command, screen_color, message, target_name, distance_m
        )
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s)
        RETURNING *;
        """,
        (
            data.device_id, data.mode, data.lat, data.lon, data.alt_m, data.speed_kmph,
            data.satellites, data.hdop, data.fix, data.temperature_c, data.pressure_hpa,
            data.acc_x, data.acc_y, data.acc_z, command["command"], command["screen_color"],
            command["message"], command["target_name"], command["distance_m"],
        ),
    )


@app.get("/")
def root():
    return {"status": "ok", "service": "SatCacher GNSS API", "docs": "/docs", "endpoints": ["/api/health", "/api/telemetry", "/api/latest", "/api/track", "/api/points", "/api/locations", "/api/assets"]}


@app.get("/api/health")
def health():
    return {"ok": True, "time_utc": datetime.now(timezone.utc).isoformat()}


@app.get("/api/locations")
def get_locations():
    locations = fetch_all(
        """
        SELECT id, name, hint, lat, lon, radius_m, play_order
        FROM game_locations
        WHERE active = TRUE
        ORDER BY play_order ASC;
        """
    )
    return {"count": len(locations), "locations": locations}


@app.post("/api/telemetry", response_model=CommandOut)
def receive_telemetry(data: TelemetryIn):
    command = build_command(data)
    save_telemetry(data, command)
    return command


@app.get("/api/latest")
def get_latest():
    row = fetch_one("SELECT * FROM telemetry_points ORDER BY id DESC LIMIT 1;")
    if row is None:
        return {"ok": False, "message": "No telemetry received yet."}
    row["ok"] = True
    if row.get("lat") is not None and row.get("lon") is not None:
        try:
            target, distance_m = get_nearest_location(row["lat"], row["lon"])
            row["target_lat"] = target["lat"]
            row["target_lon"] = target["lon"]
        except Exception:
            pass
    return row


@app.get("/api/log")
def get_full_log(limit: int = 500):
    limit = max(1, min(limit, 5000))
    rows = fetch_all("SELECT * FROM telemetry_points ORDER BY id DESC LIMIT %s;", (limit,))
    return {"count": len(rows), "points": rows}


@app.get("/api/points")
def get_points(limit: int = 1000):
    return get_track(limit=limit)


@app.get("/api/track")
def get_track(limit: int = 1000):
    limit = max(1, min(limit, 10000))
    rows = fetch_all(
        """
        SELECT id, created_at, device_id, mode, lat, lon, alt_m, speed_kmph,
               satellites, hdop, fix, temperature_c, pressure_hpa,
               acc_x, acc_y, acc_z, command, screen_color, message, target_name, distance_m
        FROM telemetry_points
        WHERE lat IS NOT NULL AND lon IS NOT NULL
        ORDER BY id DESC
        LIMIT %s;
        """,
        (limit,),
    )
    rows.reverse()
    return {"count": len(rows), "points": rows}


@app.post("/api/assets")
def create_asset(asset: AssetIn):
    row = execute_one(
        """
        INSERT INTO saved_assets (device_id, asset_type, name, lat, lon, alt_m, description, satellites, hdop)
        VALUES (%s, %s, %s, %s, %s, %s, %s, %s, %s)
        RETURNING *;
        """,
        (asset.device_id, asset.asset_type, asset.name, asset.lat, asset.lon, asset.alt_m, asset.description, asset.satellites, asset.hdop),
    )
    return {"ok": True, "asset": row}


@app.get("/api/assets")
def get_assets(limit: int = 200):
    limit = max(1, min(limit, 1000))
    rows = fetch_all("SELECT * FROM saved_assets ORDER BY id DESC LIMIT %s;", (limit,))
    return {"count": len(rows), "assets": rows}


@app.patch("/api/assets/{asset_id}")
def update_asset(asset_id: int, update: AssetUpdate):
    current = fetch_one("SELECT * FROM saved_assets WHERE id = %s;", (asset_id,))
    if current is None:
        raise HTTPException(status_code=404, detail="Saved location not found")
    new_name = update.name if update.name is not None else current["name"]
    new_type = update.asset_type if update.asset_type is not None else current["asset_type"]
    new_description = update.description if update.description is not None else current["description"]
    row = execute_one(
        """
        UPDATE saved_assets
        SET name = %s, asset_type = %s, description = %s
        WHERE id = %s
        RETURNING *;
        """,
        (new_name, new_type, new_description, asset_id),
    )
    return {"ok": True, "asset": row}


@app.delete("/api/assets/{asset_id}")
def delete_asset(asset_id: int):
    row = execute_one("DELETE FROM saved_assets WHERE id = %s RETURNING *;", (asset_id,))
    if row is None:
        raise HTTPException(status_code=404, detail="Saved location not found")
    return {"ok": True, "deleted": row}


@app.delete("/api/dev/clear-telemetry")
def clear_telemetry():
    execute("DELETE FROM telemetry_points;")
    previous_distance_by_device.clear()
    return {"ok": True, "message": "Telemetry cleared."}
