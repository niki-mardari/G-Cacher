// SatCacher frontend API config.
// It uses the current browser host, so it works on localhost and on your laptop IP.
const API_BASE_URL = (() => {
  const host = window.location.hostname || "localhost";
  return `http://${host}:8000`;
})();

function apiDocsUrl() {
  return `${API_BASE_URL}/docs`;
}
