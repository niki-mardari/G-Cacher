let scene, camera, renderer, routeGroup;

function setupScene() {
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x0b0f14);

  camera = new THREE.PerspectiveCamera(60, window.innerWidth / window.innerHeight, 0.1, 10000);
  camera.position.set(0, -220, 160);
  camera.lookAt(0, 0, 0);

  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setSize(window.innerWidth, window.innerHeight);
  document.getElementById("app").appendChild(renderer.domElement);

  const grid = new THREE.GridHelper(400, 20, 0x1d2733, 0x1d2733);
  grid.rotation.x = Math.PI / 2;
  scene.add(grid);

  const light = new THREE.DirectionalLight(0xffffff, 1.0);
  light.position.set(80, -100, 150);
  scene.add(light);
  scene.add(new THREE.AmbientLight(0xffffff, 0.35));

  routeGroup = new THREE.Group();
  scene.add(routeGroup);

  window.addEventListener("resize", () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
  });

  animate();
}

function latLonToLocal(points) {
  if (!points.length) return [];
  const lat0 = points[0].lat;
  const lon0 = points[0].lon;
  const alt0 = points[0].alt_m || 0;
  const metresPerLat = 111320;
  const metresPerLon = 111320 * Math.cos(lat0 * Math.PI / 180);

  return points.map(p => ({
    x: (p.lon - lon0) * metresPerLon,
    y: (p.lat - lat0) * metresPerLat,
    z: ((p.alt_m || alt0) - alt0) * 4,
    raw: p,
  }));
}

function clearGroup(group) {
  while (group.children.length) {
    const child = group.children.pop();
    if (child.geometry) child.geometry.dispose();
    if (child.material) child.material.dispose();
  }
}

async function loadTrack() {
  const res = await fetch(`${API_BASE_URL}/api/track?limit=2000`);
  const data = await res.json();
  const points = (data.points || []).filter(p => p.lat !== null && p.lon !== null);

  clearGroup(routeGroup);

  if (points.length < 2) {
    document.getElementById("stats").textContent = "Need at least 2 GNSS points. Send telemetry from ESP32 or dashboard simulation.";
    return;
  }

  const local = latLonToLocal(points);
  const vertices = local.map(p => new THREE.Vector3(p.x, p.y, p.z));
  const geometry = new THREE.BufferGeometry().setFromPoints(vertices);
  const material = new THREE.LineBasicMaterial({ color: 0x5ad1c5, linewidth: 3 });
  const line = new THREE.Line(geometry, material);
  routeGroup.add(line);

  const dotGeometry = new THREE.SphereGeometry(2.5, 12, 12);
  const dotMaterial = new THREE.MeshStandardMaterial({ color: 0xe2b049 });
  for (const p of local) {
    const dot = new THREE.Mesh(dotGeometry, dotMaterial);
    dot.position.set(p.x, p.y, p.z);
    routeGroup.add(dot);
  }

  const last = points[points.length - 1];
  document.getElementById("stats").textContent =
    `Points: ${points.length}\n` +
    `Latest: ${Number(last.lat).toFixed(6)}, ${Number(last.lon).toFixed(6)}\n` +
    `Altitude: ${last.alt_m ?? "—"} m\n` +
    `Satellites: ${last.satellites ?? "—"}\n` +
    `HDOP: ${last.hdop ?? "—"}`;
}

function animate() {
  requestAnimationFrame(animate);
  routeGroup.rotation.z += 0.0015;
  renderer.render(scene, camera);
}

setupScene();
document.getElementById("refreshBtn").addEventListener("click", loadTrack);
loadTrack().catch(err => {
  console.error(err);
  document.getElementById("stats").textContent = `Could not load API: ${err.message}`;
});
