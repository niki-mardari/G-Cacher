# SatCacher Backend

FastAPI backend for the SatCacher project.

The ESP32 posts telemetry to `/api/telemetry`.
The backend stores it in PostgreSQL and checks the nearest preset landmark.

The saved location feature uses `/api/assets`.
This is useful for marked spots like a parked car, pipe, cable, valve, or survey point.
