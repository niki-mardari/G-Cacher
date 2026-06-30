// API host selection:
// - Browser on laptop at http://localhost:3000 uses http://localhost:8000
// - Another device on the same Wi-Fi opening http://LAPTOP_IP:3000 uses http://LAPTOP_IP:8000
const API_BASE_URL = (() => {
  const host = window.location.hostname || "localhost";
  return `http://${host}:8000`;
})();
