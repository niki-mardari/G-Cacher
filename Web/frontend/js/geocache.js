let map;
let locationMarkers = [];
let routeLine = null;
let deviceMarker = null;
let targetMarker = null;

function fmt(value, decimals = 6) {
  return value === null || value === undefined ? '—' : Number(value).toFixed(decimals);
}
function metres(value) {
  if (value === null || value === undefined) return '—';
  return Number(value) >= 1000 ? `${(Number(value) / 1000).toFixed(2)} km` : `${Number(value).toFixed(1)} m`;
}
async function apiGet(path) {
  const res = await fetch(`${API_BASE_URL}${path}`);
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
  return res.json();
}

function initMap() {
  map = L.map('map').setView([53.2911, -6.3630], 14);
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    maxZoom: 20,
    attribution: '&copy; OpenStreetMap contributors'
  }).addTo(map);
}

async function loadLocations() {
  const data = await apiGet('/api/locations');
  locationMarkers.forEach(m => m.remove());
  locationMarkers = [];
  const body = document.getElementById('locationsBody');
  body.innerHTML = '';

  for (const loc of data.locations || []) {
    const marker = L.marker([loc.lat, loc.lon]).addTo(map)
      .bindPopup(`<b>${loc.name}</b><br>${loc.hint}<br>Radius: ${loc.radius_m} m`);
    locationMarkers.push(marker);

    const tr = document.createElement('tr');
    tr.innerHTML = `<td>${loc.name}</td><td>${loc.radius_m} m</td>`;
    tr.addEventListener('click', () => { map.setView([loc.lat, loc.lon], 16); marker.openPopup(); });
    body.appendChild(tr);
  }
}

function setCommandColour(colour) {
  const card = document.getElementById('commandCard');
  card.className = 'card command-box';
  if (colour) card.classList.add(String(colour).toLowerCase());
}

async function refresh() {
  try {
    const latest = await apiGet('/api/latest');
    const track = await apiGet('/api/track?limit=1000');
    const points = (track.points || []).filter(p => p.lat !== null && p.lon !== null);

    if (latest.ok === false) {
      document.getElementById('commandValue').textContent = 'Waiting for telemetry';
      document.getElementById('commandMessage').textContent = latest.message || 'No telemetry yet.';
      return;
    }

    document.getElementById('commandValue').textContent = latest.command || latest.title || '—';
    document.getElementById('commandMessage').textContent = latest.message || '—';
    document.getElementById('latValue').textContent = fmt(latest.lat, 6);
    document.getElementById('lonValue').textContent = fmt(latest.lon, 6);
    document.getElementById('satValue').textContent = latest.satellites ?? '—';
    document.getElementById('hdopValue').textContent = latest.hdop ?? '—';
    document.getElementById('targetValue').textContent = latest.target_name || '—';
    document.getElementById('distanceValue').textContent = metres(latest.distance_m);
    setCommandColour(latest.screen_color);

    if (points.length > 0) {
      const latlngs = points.map(p => [p.lat, p.lon]);
      if (routeLine) routeLine.setLatLngs(latlngs);
      else routeLine = L.polyline(latlngs, { weight: 4, opacity: 0.8 }).addTo(map);

      const last = points[points.length - 1];
      if (deviceMarker) deviceMarker.setLatLng([last.lat, last.lon]);
      else deviceMarker = L.circleMarker([last.lat, last.lon], { radius: 9, weight: 3, fillOpacity: 1 }).addTo(map);
      deviceMarker.bindPopup(`<b>Latest SatCacher position</b><br>Lat: ${fmt(last.lat)}<br>Lon: ${fmt(last.lon)}<br>Sats: ${last.satellites ?? '—'}`);
    }

    if (latest.target_lat && latest.target_lon) {
      if (targetMarker) targetMarker.setLatLng([latest.target_lat, latest.target_lon]);
      else targetMarker = L.marker([latest.target_lat, latest.target_lon]).addTo(map);
      targetMarker.bindPopup(`<b>${latest.target_name || 'Target'}</b><br>Distance: ${metres(latest.distance_m)}`);
    }
  } catch (err) {
    console.error(err);
    document.getElementById('commandValue').textContent = 'Backend not reachable';
    document.getElementById('commandMessage').textContent = `Could not reach ${API_BASE_URL}. Check Docker.`;
  }
}

async function clearTelemetry() {
  if (!confirm('Clear local telemetry test data?')) return;
  await fetch(`${API_BASE_URL}/api/dev/clear-telemetry`, { method: 'DELETE' });
  if (routeLine) routeLine.remove();
  if (deviceMarker) deviceMarker.remove();
  if (targetMarker) targetMarker.remove();
  routeLine = deviceMarker = targetMarker = null;
  await refresh();
}

initMap();
document.getElementById('refreshBtn').addEventListener('click', refresh);
document.getElementById('clearBtn').addEventListener('click', clearTelemetry);
loadLocations().then(refresh);
setInterval(refresh, 5000);
