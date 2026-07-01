const REFRESH_MS = 3000;

let map;
let locationMarkers = [];
let assetMarkers = [];
let latestMarker = null;
let routeLine = null;
let selectedSimPosition = null;
let selectedMarker = null;
let latestPoint = null;
let lastAssetRefresh = 0;

function metresToText(m) {
  if (m === null || m === undefined || Number.isNaN(Number(m))) return "—";
  const n = Number(m);
  if (n >= 1000) return `${(n / 1000).toFixed(2)} km`;
  return `${n.toFixed(1)} m`;
}

function formatTime(value) {
  if (!value) return "—";
  try { return new Date(value).toLocaleTimeString(); }
  catch { return value; }
}

function commandClass(screenColor) {
  const allowed = ["blue", "red", "green", "orange", "yellow"];
  return allowed.includes(screenColor) ? screenColor : "";
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

async function apiPatch(path, body) {
  const res = await fetch(`${API_BASE_URL}${path}`, {
    method: "PATCH",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
  return res.json();
}

async function apiDelete(path) {
  const res = await fetch(`${API_BASE_URL}${path}`, { method: "DELETE" });
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
  return res.json();
}

function initLinks() {
  document.getElementById("docsLink").href = `${API_BASE_URL}/docs`;
  document.getElementById("docsCardLink").href = `${API_BASE_URL}/docs`;
}

function initMap() {
  map = L.map("map").setView([53.2911, -6.3630], 15);

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
        color: "#102030",
        fillColor: "#e0a800",
        fillOpacity: 1,
        weight: 2,
      }).addTo(map).bindPopup("Selected simulated position");
    }
  });
}

async function loadLocations() {
  const data = await apiGet("/api/locations");

  locationMarkers.forEach(marker => marker.remove());
  locationMarkers = [];

  for (const location of data.locations || []) {
    const marker = L.marker([location.lat, location.lon])
      .addTo(map)
      .bindPopup(`<b>${location.name}</b><br>${location.hint}<br>Radius: ${location.radius_m} m`);
    locationMarkers.push(marker);
  }
}

function renderTrack(points) {
  const valid = points.filter(p => p.lat !== null && p.lon !== null);
  if (valid.length === 0) return;

  const latlngs = valid.map(p => [p.lat, p.lon]);
  if (routeLine) routeLine.setLatLngs(latlngs);
  else {
    routeLine = L.polyline(latlngs, {
      color: "#e87522",
      weight: 4,
      opacity: 0.85,
    }).addTo(map);
  }

  const latest = valid[valid.length - 1];
  latestPoint = latest;

  if (latestMarker) latestMarker.setLatLng([latest.lat, latest.lon]);
  else {
    latestMarker = L.circleMarker([latest.lat, latest.lon], {
      radius: 9,
      color: "#102030",
      fillColor: "#e87522",
      fillOpacity: 1,
      weight: 3,
    }).addTo(map);
  }

  latestMarker.setPopupContent(
    `<b>Latest Sat Cacher position</b><br>` +
    `Lat: ${Number(latest.lat).toFixed(6)}<br>` +
    `Lon: ${Number(latest.lon).toFixed(6)}<br>` +
    `Sats: ${latest.satellites ?? "—"}<br>` +
    `HDOP: ${latest.hdop ?? "—"}<br>` +
    `Command: ${latest.command ?? "—"}`
  );
}

function updateGamePanel(latest) {
  const box = document.getElementById("commandBox");
  const color = latest.screen_color || "";
  box.className = `status-box ${commandClass(color)}`;

  document.getElementById("commandText").textContent = latest.command || "Waiting";
  document.getElementById("commandMessage").textContent = latest.message || "Waiting for telemetry.";
  document.getElementById("targetName").textContent = latest.target_name || latest.title || "—";
  document.getElementById("targetDistance").textContent = metresToText(latest.distance_m);

  if (latest.lat !== null && latest.lon !== null && latest.lat !== undefined && latest.lon !== undefined) {
    document.getElementById("latestLatLon").textContent = `${Number(latest.lat).toFixed(6)}, ${Number(latest.lon).toFixed(6)}`;
  } else {
    document.getElementById("latestLatLon").textContent = "—";
  }

  document.getElementById("latestSats").textContent = latest.satellites ?? "—";
  document.getElementById("latestHdop").textContent = latest.hdop ?? "—";
  document.getElementById("serverStatus").textContent = "OK";
}

function renderLog(points) {
  const body = document.getElementById("logBody");
  body.innerHTML = "";

  const latestFirst = [...points].reverse().slice(0, 40);
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

async function loadAssets() {
  const data = await apiGet("/api/assets?limit=500");
  const assets = data.assets || [];

  assetMarkers.forEach(marker => marker.remove());
  assetMarkers = [];

  for (const asset of assets) {
    const marker = L.circleMarker([asset.lat, asset.lon], {
      radius: 8,
      color: "#102030",
      fillColor: "#238b5f",
      fillOpacity: 0.9,
      weight: 2,
    }).addTo(map).bindPopup(
      `<b>${asset.name}</b><br>` +
      `Type: ${asset.asset_type}<br>` +
      `Lat: ${Number(asset.lat).toFixed(6)}<br>` +
      `Lon: ${Number(asset.lon).toFixed(6)}<br>` +
      `${asset.description || ""}`
    );
    assetMarkers.push(marker);
  }

  renderAssetList(assets);
  lastAssetRefresh = Date.now();
}

function renderAssetList(assets) {
  const container = document.getElementById("assetList");
  container.innerHTML = "";

  if (assets.length === 0) {
    container.textContent = "No saved locations yet.";
    return;
  }

  for (const asset of assets) {
    const card = document.createElement("div");
    card.className = "asset-card";
    card.innerHTML = `
      <div>
        <input value="${escapeHtml(asset.name)}" data-field="name" aria-label="Location name">
        <div class="asset-meta">ID ${asset.id} · ${Number(asset.lat).toFixed(6)}, ${Number(asset.lon).toFixed(6)}</div>
      </div>
      <input value="${escapeHtml(asset.asset_type)}" data-field="asset_type" aria-label="Location tag">
      <input value="${escapeHtml(asset.description || "")}" data-field="description" aria-label="Description">
      <div class="button-row">
        <button class="small" data-action="save">Save</button>
        <button class="small danger" data-action="delete">Delete</button>
      </div>
    `;

    card.querySelector('[data-action="save"]').addEventListener("click", async () => {
      const body = {
        name: card.querySelector('[data-field="name"]').value,
        asset_type: card.querySelector('[data-field="asset_type"]').value,
        description: card.querySelector('[data-field="description"]').value,
      };
      await apiPatch(`/api/assets/${asset.id}`, body);
      await loadAssets();
    });

    card.querySelector('[data-action="delete"]').addEventListener("click", async () => {
      if (!confirm(`Delete saved location: ${asset.name}?`)) return;
      await apiDelete(`/api/assets/${asset.id}`);
      await loadAssets();
    });

    container.appendChild(card);
  }
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

async function refreshDashboard() {
  try {
    const latest = await apiGet("/api/latest");
    if (latest.ok !== false) updateGamePanel(latest);

    const track = await apiGet("/api/track?limit=1000");
    const points = track.points || [];
    renderTrack(points);
    renderLog(points);
    if (Date.now() - lastAssetRefresh > 12000) {
      await loadAssets();
      lastAssetRefresh = Date.now();
    }
  } catch (err) {
    console.error("Refresh failed:", err);
    document.getElementById("serverStatus").textContent = "Backend not reachable";
    document.getElementById("commandText").textContent = "Backend not reachable";
    document.getElementById("commandMessage").textContent = `Could not reach ${API_BASE_URL}. Start Docker and check backend logs.`;
  }
}

async function sendSimulatedTelemetry() {
  if (!selectedSimPosition) {
    alert("Click the map first to select a simulated position.");
    return;
  }

  await apiPost("/api/telemetry", {
    device_id: "browser_simulator",
    mode: "geocache",
    lat: selectedSimPosition.lat,
    lon: selectedSimPosition.lon,
    alt_m: 42.0,
    speed_kmph: 2.0,
    satellites: 35,
    hdop: 0.8,
    fix: true,
    temperature_c: 30.0,
    pressure_hpa: 1012.4,
    acc_x: 0.1,
    acc_y: 0.0,
    acc_z: 9.8,
  });
  await refreshDashboard();
}

async function saveLatestPoint() {
  if (!latestPoint || latestPoint.lat === null || latestPoint.lon === null) {
    alert("No latest GNSS point is available yet.");
    return;
  }

  await apiPost("/api/assets", {
    device_id: latestPoint.device_id || "dashboard",
    asset_type: "marker",
    name: "Saved dashboard point",
    lat: latestPoint.lat,
    lon: latestPoint.lon,
    alt_m: latestPoint.alt_m,
    description: "Saved from the web dashboard. Rename/tag this location here.",
    satellites: latestPoint.satellites,
    hdop: latestPoint.hdop,
  });
  await loadAssets();
}

async function clearTelemetry() {
  if (!confirm("Clear telemetry points in the local dev database?")) return;
  await fetch(`${API_BASE_URL}/api/dev/clear-telemetry`, { method: "DELETE" });
  if (routeLine) routeLine.setLatLngs([]);
  if (latestMarker) latestMarker.remove();
  latestMarker = null;
  latestPoint = null;
  await refreshDashboard();
}

async function main() {
  initLinks();
  initMap();

  document.getElementById("simulateBtn").addEventListener("click", sendSimulatedTelemetry);
  document.getElementById("clearBtn").addEventListener("click", clearTelemetry);
  document.getElementById("saveLatestBtn").addEventListener("click", saveLatestPoint);

  await loadLocations();
  await refreshDashboard();
  setInterval(refreshDashboard, REFRESH_MS);
}

main();
