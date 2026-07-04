# SatCacher Web Server

This folder runs the SatCacher web project using Docker.

The project has three containers:

- `frontend` - a simple Nginx website on port `3000`
- `backend` - a FastAPI server on port `8000`
- `db` - a PostgreSQL database on port `5432`

## Pages

Open these after Docker starts:

- Home menu: `http://localhost:3000`
- Geocache game: `http://localhost:3000/geocache.html`
- 3D viewer: `http://localhost:3000/terrain.html`
- Saved locations: `http://localhost:3000/saved-locations.html`
- API docs: `http://localhost:8000/docs`

The home page is intentionally simple. It only shows the four main project features.

## How to run

From this `Web` folder:

```bash
docker compose up --build -d
```

Check containers:

```bash
docker compose ps
```

Stop everything:

```bash
docker compose down
```

Reset the database if needed:

```bash
docker compose down -v
docker compose up --build -d
```

Only use `down -v` when it is okay to delete local test data.

## Main API endpoints

- `POST /api/telemetry` - ESP32 sends GNSS data here
- `GET /api/latest` - latest received GNSS point
- `GET /api/track` - route/track points
- `GET /api/points` - same data for the 3D viewer
- `GET /api/locations` - preset geocache landmarks
- `GET /api/assets` - saved marked locations
- `POST /api/assets` - save a new marked location
- `PATCH /api/assets/{id}` - rename/tag a saved location
- `DELETE /api/assets/{id}` - delete a saved location
- `http://localhost:8000/api/log` - To check the data points in the database 

## Project idea

SatCacher is a GNSS field tool. The ESP32-S3 reads location from the LC29H module and sends it to this backend. The website then shows the route, geocache landmarks, saved field locations, and a 3D viewer.
