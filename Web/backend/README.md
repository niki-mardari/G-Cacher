# Backend

This is the FastAPI backend for Sat Cacher.

Main file:

```text
backend/app/main.py
```

It receives telemetry from the ESP32-S3 at:

```text
POST /api/telemetry
```

It stores points in PostgreSQL and returns the current geocache command.

It also saves marked locations/assets at:

```text
POST /api/assets
```

Run through Docker Compose from the `Web` folder.
