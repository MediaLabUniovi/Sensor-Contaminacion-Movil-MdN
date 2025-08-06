document.addEventListener('DOMContentLoaded', function () {
  // URL base del backend
  const API_BASE = "http://localhost:80";
  const rideSelect = document.getElementById("rideSelect");
  const statsDiv = document.getElementById("statsContent");
  const statsPanel = document.getElementById("stats");
  const dateFilter = document.getElementById("dateFilter");

  // Función para mostrar estadísticas
  function setStats(html) {
    const titulo = `<b style="color:#0077cc;font-size:1.08rem;">Estadísticas</b><br>`;
    if (window.innerWidth <= 500) {
      // Móvil: muestra título azul
      statsDiv.innerHTML = titulo + html;
      statsPanel.innerHTML = '';
    } else {
      // Escritorio
      statsPanel.innerHTML = titulo + html;
      statsDiv.innerHTML = '';
    }
  }

  // Inicialización del mapa centrado en Oviedo
  const map = L.map('map').setView([43.36, -5.84], 13);

  // Capa base de OpenStreetMap
  L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
    attribution: 'Map data &copy; OpenStreetMap contributors'
  }).addTo(map);

  let routeLayer = null;  // Capa de la ruta activa
  let allRides = [];      // Lista de rutas cargadas

  // Iconos personalizados para inicio y fin de ruta
  const startIcon = new L.Icon({
    iconUrl: 'https://cdn-icons-png.flaticon.com/512/3187/3187666.png',
    iconSize: [32, 32],
    iconAnchor: [16, 32]
  });
  const endIcon = new L.Icon({
    iconUrl: 'https://cdn-icons-png.flaticon.com/512/1894/1894418.png',
    iconSize: [32, 32],
    iconAnchor: [16, 32]
  });

  // Devuelve color según la concentración de partículas PM2.5
  function pmColor(pm) {
    if (pm < 12) return 'green';
    else if (pm < 35) return 'orange';
    else return 'red';
  }

  // Carga todas las rutas recientes del backend
  function loadRides() {
    fetch(`${API_BASE}/rides.php?days=30`)
      .then(res => res.json())
      .then(data => {
        allRides = data;
        renderRideOptions(data);
      });
  }

  // Pinta el combo de selección de rutas
  function renderRideOptions(data) {
    rideSelect.innerHTML = "";
    setStats(data.length === 0 ? "No hay rutas disponibles" : "Selecciona una ruta");
    data.forEach(ride => {
      const option = document.createElement("option");
      option.value = ride.id;
      option.text = `${ride.created_at.slice(0, 10)} - ${Number(ride.distance_km).toFixed(2)} km`;
      rideSelect.appendChild(option);
    });
    if (data.length > 0) loadRide(data[0].id);  // Carga la primera ruta por defecto
  }

  // Muestra las estadísticas de una ruta
  function updateStats(stats) {
    if (!stats) {
      setStats("No hay datos");
      return;
    }
    const html = `
      Fecha: ${stats.created_at?.slice(0,10)}<br>
      Distancia: ${Number(stats.distance_km).toFixed(2)} km<br>
      Duración: ${stats.duration} min<br>
      Temp media: ${Number(stats.avg_temp).toFixed(1)} °C<br>
      PM2.5 media: ${Number(stats.avg_pm2_5).toFixed(1)}<br>
      PM10 media: ${Number(stats.avg_pm10).toFixed(1)}
    `;
    setStats(html);
  }

  // Carga y dibuja una ruta concreta en el mapa
  function loadRide(rideId) {
    fetch(`${API_BASE}/rides.php?r=${rideId}`)
      .then(res => res.json())
      .then(data => {
        if (routeLayer) routeLayer.remove();
        if (!data || data.length === 0) {
          setStats("Ruta vacía");
          return;
        }

        const lines = [];
        const markers = [];

        // Dibuja cada segmento de la ruta como línea coloreada por PM2.5
        for (let i = 0; i < data.length - 1; i++) {
          const p1 = data[i];
          const p2 = data[i + 1];
          const line = L.polyline([[p1.lat, p1.lon], [p2.lat, p2.lon]], {
            color: pmColor(p1.pm2_5),
            weight: 6,
            opacity: 0.8
          });
          // Al hacer clic en un tramo, muestra estadísticas de la ruta
          line.on("click", () => updateStats(allRides.find(r => r.id === rideId)));
          lines.push(line);
        }

        // Marca el inicio y fin de ruta
        markers.push(L.marker([data[0].lat, data[0].lon], { icon: startIcon }));
        markers.push(L.marker([data[data.length - 1].lat, data[data.length - 1].lon], { icon: endIcon }));

        // Agrupa líneas y marcadores en una sola capa y la añade al mapa
        routeLayer = L.layerGroup([...lines, ...markers]);
        routeLayer.addTo(map);
        map.fitBounds(L.featureGroup(lines).getBounds());

        let rideStats = allRides.find(r => Number(r.id) === Number(rideId))
        if (rideStats) updateStats(rideStats);
      });
  }

   // Al cambiar la ruta seleccionada, se carga en el mapa
  rideSelect.addEventListener("change", () => loadRide(rideSelect.value));

  // Filtra rutas por fecha y las pinta en el mapa
  dateFilter.addEventListener("change", () => {
    const selectedDate = dateFilter.value;
    if (routeLayer) routeLayer.remove();
    if (!selectedDate) {
      renderRideOptions(allRides);
      return;
    }

    const filtered = allRides.filter(r => r.created_at.startsWith(selectedDate));

    if (filtered.length === 0) {
      setStats("⚠️ No hay rutas en este día");
      return;
    }

    const allLayers = [];
    const allMarkers = [];

    filtered.forEach(ride => {
      fetch(`${API_BASE}/rides.php?r=${ride.id}`)
        .then(res => res.json())
        .then(data => {
          const lines = [];
          for (let i = 0; i < data.length - 1; i++) {
            const p1 = data[i];
            const p2 = data[i + 1];
            const line = L.polyline([[p1.lat, p1.lon], [p2.lat, p2.lon]], {
              color: pmColor(p1.pm2_5),
              weight: 6,
              opacity: 0.7
            }).addTo(map);
            line.on("click", () => updateStats(allRides.find(r => r.id === ride.id)));
            lines.push(line);
          }
          allLayers.push(...lines);
          allMarkers.push(L.marker([data[0].lat, data[0].lon], { icon: startIcon }).addTo(map));
          allMarkers.push(L.marker([data[data.length - 1].lat, data[data.length - 1].lon], { icon: endIcon }).addTo(map));

          if (filtered.indexOf(ride) === filtered.length - 1) {
            routeLayer = L.layerGroup([...allLayers, ...allMarkers]);
            routeLayer.addTo(map);
            map.fitBounds(L.featureGroup(allLayers).getBounds());
            setStats(`🗺️ Rutas cargadas: ${filtered.length}<br>Haz clic en una para ver sus estadísticas.`);
          }
        });
    });
  });

  // Botón para limpiar el mapa y resetear filtros
  const clearBtn = document.createElement("button");
  clearBtn.textContent = "🧹 Limpiar mapa";
  clearBtn.style.marginLeft = "1rem";
  clearBtn.onclick = () => {
    map.eachLayer(layer => {
      if (!layer._url) map.removeLayer(layer);  // No borra la capa base de OpenStreetMap
    });
    routeLayer = null;
    rideSelect.selectedIndex = 0;
    dateFilter.value = "";
    setStats("Mapa limpiado");
  };
  document.getElementById("clearMap").onclick = clearBtn.onclick;
  // Carga las rutas al iniciar
  loadRides();
});

// Función para recolocar el panel de estadísticas en móvil
function relocateStatsPanelIfMobile() {
  const stats = document.getElementById('stats');
  const map = document.getElementById('map');
  if (!stats || !map) return;
  if (window.innerWidth <= 500) {
    // Si la pantalla es pequeña, mueve el panel debajo del mapa
    if (stats.parentElement === map) {
      map.parentNode.insertBefore(stats, map.nextSibling);
    }
  } else {
    // En escritorio, lo vuelve a meter dentro del mapa
    if (stats.parentElement !== map) {
      map.appendChild(stats);
    }
    // Limpia el style en escritorio
    stats.removeAttribute("style");
  }
}
window.addEventListener('DOMContentLoaded', relocateStatsPanelIfMobile);
window.addEventListener('resize', relocateStatsPanelIfMobile);
