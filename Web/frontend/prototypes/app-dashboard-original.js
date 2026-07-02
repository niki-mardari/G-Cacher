const CLAIM_RADIUS_M = 30;
const REFRESH_MS = 3000;

let map;
let cacheMarkers = [];
let routeLine = null;
let latestMarker = null;
let selectedSimPosition = null;
let selectedMarker = null;
let assetMarkers = [];

function metresToText(m) {
  if (m === null || m === undefined) return "—";
  if (m >= 1000) return `${(m / 1000).toFixed(2)} km`;
  return `${Number(m).toFixed(1)} m`;
}

function formatTime(value) {
  if (!value) return "—";
  try {
    return new Date(value).toLocaleTimeString();
  } catch {
    return value;
  }
}

function initMap() {
  map = L.map("map").setView([53.3440, -6.2630], 14);

  L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
    attribution: "© OpenStreetMap contributors",
    maxZoom: 19,
  }).addTo(map);

  map.on("click", e => {
    selectedSimPosition = { lat: e.latlng.lat, lon: e.latlng.lng };

    if (selectedMarker) {
      selectedMarker.setLatLng([selectedSimPosition.lat, selectedSimPosition.lon]);
    } else {
      selectedMarker = L.circleMarker([selectedSimPosition.lat, selectedSimPosition.lon], {
        radius: 7,
        color: "#0d1b2a",
        fillColor: "#ffb703",
        fillOpacity: 1,
        weight: 2,
      }).addTo(map).bindPopup("Selected simulated position");
    }
  });
}

async function apiGet(path) {
  const res = await fetch(`${API_BASE_URL}${path}`);
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
  return res.json();
}

async function apiPost(path, body) {
  const res = await fetch(`${API_BASE_URL}${path}`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
  return res.json();
}

async function loadLocations() {
  const data = await apiGet("/api/locations");

  cacheMarkers.forEach(marker => marker.remove());
  cacheMarkers = [];

  for (const location of data.locations || []) {
    const marker = L.marker([location.lat, location.lon])
      .addTo(map)
      .bindPopup(`<b>${location.name}</b><br>${location.hint}<br>Radius: ${location.radius_m} m`);
    cacheMarkers.push(marker);
  }
}

function commandClass(screenColor) {
  const allowed = ["blue", "red", "green", "orange", "yellow"];
  return allowed.includes(screenColor) ? screenColor : "";
}

function updateCommandCard(latest) {
  const device = document.getElementById("device");
  const title = latest.title || latest.command || "No command yet";
  const message = latest.message || "Waiting for telemetry.";
  const color = latest.screen_color || "";

  device.className = `device ${commandClass(color)}`;
  document.getElementById("sigMain").textContent = color ? color.toUpperCase() : "—";
  document.getElementById("sigSub").textContent = latest.command || "No server command yet";
  document.getElementById("commandTitle").textContent = title;
  document.getElementById("commandMessage").textContent = message;

  document.getElementById("latestLatLon").textContent =
    latest.lat && latest.lon ? `${Number(latest.lat).toFixed(6)}, ${Number(latest.lon).toFixed(6)}` : "—";
  document.getElementById("latestSats").textContent = latest.satellites ?? "—";
  document.getElementById("latestHdop").textContent = latest.hdop ?? "—";
  document.getElementById("latestDistance").textContent = metresToText(latest.distance_m);
}

function renderTrack(points) {
  const valid = points.filter(p => p.lat !== null && p.lon !== null);
  if (valid.length === 0) return;

  const latlngs = valid.map(p => [p.lat, p.lon]);

  if (routeLine) {
    routeLine.setLatLngs(latlngs);
  } else {
    routeLine = L.polyline(latlngs, {
      color: "#ff6b35",
      weight: 4,
      opacity: 0.8,
    }).addTo(map);
  }

  const latest = valid[valid.length - 1];
  if (latestMarker) {
    latestMarker.setLatLng([latest.lat, latest.lon]);
  } else {
    latestMarker = L.circleMarker([latest.lat, latest.lon], {
      radius: 9,
      color: "#0d1b2a",
      fillColor: "#ff6b35",
      fillOpacity: 1,
      weight: 3,
    }).addTo(map).bindPopup("Latest ESP32-S3 position");
  }

  latestMarker.setPopupContent(
    `<b>Latest ESP32-S3 position</b><br>` +
    `Lat: ${Number(latest.lat).toFixed(6)}<br>` +
    `Lon: ${Number(latest.lon).toFixed(6)}<br>` +
    `Sats: ${latest.satellites ?? "—"}<br>` +
    `HDOP: ${latest.hdop ?? "—"}<br>` +
    `Command: ${latest.command ?? "—"}`
  );
}

function renderLog(points) {
  const body = document.getElementById("logBody");
  body.innerHTML = "";

  const latestFirst = [...points].reverse().slice(0, 50);
  for (const p of latestFirst) {
    const row = document.createElement("tr");
    row.innerHTML = `
      <td>${p.id}</td>
      <td>${formatTime(p.created_at)}</td>
      <td>${p.lat !== null ? Number(p.lat).toFixed(6) : "—"}</td>
      <td>${p.lon !== null ? Number(p.lon).toFixed(6) : "—"}</td>
      <td>${p.satellites ?? "—"}</td>
      <td>${p.hdop ?? "—"}</td>
      <td>${p.command ?? "—"}</td>
    `;
    body.appendChild(row);
  }
}

async function refreshDashboard() {
  try {
    const latest = await apiGet("/api/latest");
    if (latest.ok !== false) updateCommandCard(latest);

    const track = await apiGet("/api/track?limit=1000");
    const points = track.points || [];
    renderTrack(points);
    renderLog(points);
  } catch (err) {
    console.error("Dashboard refresh failed:", err);
    document.getElementById("commandTitle").textContent = "Backend not reachable";
    document.getElementById("commandMessage").textContent = `Could not reach ${API_BASE_URL}. Check Docker Compose and backend logs.`;
  }
}

async function sendSimulatedTelemetry() {
  if (!selectedSimPosition) {
    alert("Click the map first to select a simulated ESP32 position.");
    return;
  }

  const body = {
    device_id: "browser_simulator",
    mode: "geocache",
    lat: selectedSimPosition.lat,
    lon: selectedSimPosition.lon,
    alt_m: 42.0,
    speed_kmph: 2.0,
    satellites: 35,
    hdop: 0.8,
    fix: true,
    temperature_c: 39.8,
    pressure_hpa: 1012.4,
    acc_x: 0.1,
    acc_y: 0.0,
    acc_z: 9.8,
  };

  await apiPost("/api/telemetry", body);
  await refreshDashboard();
}

async function clearTelemetry() {
  if (!confirm("Clear the telemetry log in the local dev database?")) return;
  await fetch(`${API_BASE_URL}/api/dev/clear-telemetry`, { method: "DELETE" });
  if (routeLine) routeLine.setLatLngs([]);
  if (latestMarker) latestMarker.remove();
  latestMarker = null;
  await refreshDashboard();
}

async function main() {
  initMap();
  document.getElementById("simulateBtn").addEventListener("click", sendSimulatedTelemetry);
  document.getElementById("clearBtn").addEventListener("click", clearTelemetry);

  await loadLocations();
  await refreshDashboard();
  setInterval(refreshDashboard, REFRESH_MS);
}

main();
