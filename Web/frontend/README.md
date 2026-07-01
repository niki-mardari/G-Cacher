# Frontend

This is the Sat Cacher website.

Main pages:

```text
index.html       Main dashboard, geocache game, saved locations, map
terrain.html     New 3D route viewer
```

Important files:

```text
js/config.js     Chooses the API host automatically
js/app.js        Dashboard logic
css/style.css    Dashboard styling
```

The frontend is served by Nginx in Docker on port `3000`.
