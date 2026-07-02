# SatCacher Frontend

The frontend is kept simple on purpose.

## Main files

- `index.html` - simple home menu
- `geocache.html` - geocache game map
- `saved-locations.html` - saved location markers and editing
- `terrain.html` - new 3D viewer prototype
- `prototypes/terrain-original.html` - old terrain page kept as backup
- `prototypes/3D_Modelling_Test_Sample.html` - teammate prototype copy

## JavaScript

- `js/config.js` - chooses the backend URL based on the current browser host
- `js/geocache.js` - loads latest telemetry and landmarks
- `js/saved-locations.js` - loads and edits saved field locations

The pages use Leaflet from a CDN for maps.
