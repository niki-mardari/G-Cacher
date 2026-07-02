let map;
let markers = [];
let assets = [];

function fmt(v, d = 6) { return v === null || v === undefined ? '—' : Number(v).toFixed(d); }
function assetClass(type) {
  const t = String(type || 'marker').toLowerCase().replace(/[^a-z0-9_-]/g, '');
  const known = ['car', 'pipe', 'cable', 'survey', 'construction', 'marker', 'other'];
  return known.includes(t) ? t : 'other';
}
function iconFor(type) {
  const cls = assetClass(type);
  return L.divIcon({ className: '', html: `<div class="marker-dot marker-${cls}"></div>`, iconSize: [18,18], iconAnchor: [9,9] });
}
async function apiGet(path) {
  const res = await fetch(`${API_BASE_URL}${path}`);
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
  return res.json();
}
async function apiPatch(path, body) {
  const res = await fetch(`${API_BASE_URL}${path}`, { method: 'PATCH', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body) });
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
  return res.json();
}
async function apiDelete(path) {
  const res = await fetch(`${API_BASE_URL}${path}`, { method: 'DELETE' });
  if (!res.ok) throw new Error(`${res.status} ${res.statusText}`);
  return res.json();
}
function initMap() {
  map = L.map('map').setView([53.2911, -6.3630], 14);
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', { maxZoom: 20, attribution: '&copy; OpenStreetMap contributors' }).addTo(map);
}
function selectAsset(asset) {
  document.getElementById('assetId').value = asset.id;
  document.getElementById('assetName').value = asset.name || '';
  document.getElementById('assetType').value = assetClass(asset.asset_type);
  document.getElementById('assetDescription').value = asset.description || '';
  document.getElementById('selectedHint').textContent = `Selected ID ${asset.id}: ${asset.lat.toFixed(6)}, ${asset.lon.toFixed(6)}`;
}
async function loadAssets() {
  const data = await apiGet('/api/assets?limit=1000');
  assets = data.assets || [];
  markers.forEach(m => m.remove());
  markers = [];
  const body = document.getElementById('assetsBody');
  body.innerHTML = '';

  for (const asset of assets) {
    const marker = L.marker([asset.lat, asset.lon], { icon: iconFor(asset.asset_type) }).addTo(map);
    marker.bindPopup(`<b>${asset.name}</b><br>Type: ${asset.asset_type}<br>${asset.description || ''}<br>Lat: ${fmt(asset.lat)}<br>Lon: ${fmt(asset.lon)}`);
    marker.on('click', () => selectAsset(asset));
    markers.push(marker);

    const tr = document.createElement('tr');
    tr.innerHTML = `<td>${asset.id}</td><td>${asset.name}</td><td>${asset.asset_type}</td><td>${fmt(asset.lat)}</td><td>${fmt(asset.lon)}</td>`;
    tr.addEventListener('click', () => { selectAsset(asset); map.setView([asset.lat, asset.lon], 17); marker.openPopup(); });
    body.appendChild(tr);
  }

  if (assets.length > 0) {
    const group = L.featureGroup(markers);
    map.fitBounds(group.getBounds().pad(0.2));
  }
}
async function saveSelected() {
  const id = document.getElementById('assetId').value;
  if (!id) { alert('Select a saved location first.'); return; }
  const body = {
    name: document.getElementById('assetName').value,
    asset_type: document.getElementById('assetType').value,
    description: document.getElementById('assetDescription').value
  };
  await apiPatch(`/api/assets/${id}`, body);
  await loadAssets();
}
async function deleteSelected() {
  const id = document.getElementById('assetId').value;
  if (!id) { alert('Select a saved location first.'); return; }
  if (!confirm(`Delete saved location ${id}?`)) return;
  await apiDelete(`/api/assets/${id}`);
  document.getElementById('assetId').value = '';
  document.getElementById('assetName').value = '';
  document.getElementById('assetDescription').value = '';
  document.getElementById('selectedHint').textContent = 'Select a marker from the map or table.';
  await loadAssets();
}

initMap();
document.getElementById('refreshBtn').addEventListener('click', loadAssets);
document.getElementById('saveBtn').addEventListener('click', saveSelected);
document.getElementById('deleteBtn').addEventListener('click', deleteSelected);
loadAssets().catch(err => console.error(err));
