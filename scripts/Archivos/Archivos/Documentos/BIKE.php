<?php
    /*//Comprobación user y pwd
    include ("./seguridad.php");
    $link=conectarse();
    
    $aux=nuevavisita ($link, 'medialab_proyectos_b','if');

    $lang=$_GET["lang"];
	if ($lang=="")
	{
		$lang="es";
	}
	$estaweb="congresoIF";
	$texto=cargartextos ($link, $estaweb, $lang);
*/
?>

<!doctype html>
<html lang="es">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css" />
    <link rel="stylesheet" href="css/style_bike.css" />
</head>
<body>
<?php
// Incluye cabecera visual (menú, logotipos, etc.)
 include 'head.php';
 include 'cabecera.php';
?>

<!-- PORTADA -->
<section class="portada-principal" style="background-image: url('media/imagenes/portada_bici.png'); background-size: cover; background-position: center; padding: 100px 0;">
    <div style="max-width: 360px; margin-left: 6%; background: white; padding: 40px 30px; box-shadow: 0 4px 24px rgba(0,0,0,0.25); text-align: center;">
        <i class="fas fa-quote-left" style="font-size: 26px; color: #f45b0d;"></i>
        <h2 style="margin-top: 20px; font-size: 22px; color: #1d1d1d; line-height: 1.6;">
            Un proyecto en colaboración con Mar de Niebla que te permite visualizar la contaminación del aire a lo largo de tus rutas en bicicleta.
        </h2>
    </div>
</section>


<!-- INICIO SECCIÓN VISOR BICI -->
<main id="visor-bici">
        <div id="panel">
            <div class="filter-panel">
                <div class="filter-group">
                    <label for="dateFilter">Fecha</label>
                    <input type="date" id="dateFilter">
                </div>

                <div class="filter-group">
                    <label for="rideSelect">Ruta</label>
                    <select id="rideSelect"></select>
                </div>

                <div class="filter-group">
                    <label style="opacity: 0;">Acción</label>
                    <button id="clearMap" type="button">🧹 Limpiar mapa</button>
                </div>
            </div>
        </div>
        <div id="map">
            <!-- Panel flotante de Estadísticas SOLO sobre el mapa -->
            <div id="stats"></div>
            <!-- Leyenda SOLO sobre el mapa -->
            <div id="legend">
                <b>Leyenda PM2.5:</b>
                <div style="display:flex;align-items:center;gap:8px;margin-top:5px">
                    <span style="width:24px;height:12px;background:green;display:inline-block"></span> &lt;12
                    <span style="width:24px;height:12px;background:orange;display:inline-block"></span> 12–35
                    <span style="width:24px;height:12px;background:red;display:inline-block"></span> &gt;35
                </div>
            </div>
        </div>
        <div id="statsContent" class="mt-4"></div>
</main>
<!-- FIN SECCIÓN VISOR BICI -->

<?php
 include 'pie_b.php';
 include 'pie_JS.php';
?>

<!-- JS AL FINAL -->
<script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
<script src="main.js"></script>
</body>
</html>
