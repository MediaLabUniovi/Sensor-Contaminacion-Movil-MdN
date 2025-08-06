<?php
require_once 'db.php';          // Conexión a la base de datos
require_once 'haversine.php';   // Función para calcular distancias

// Solo procesa si la petición es POST y se subió un archivo csv
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_FILES['csv'])) {
    $csv = $_FILES['csv']['tmp_name'];
    $measurements = [];

    // Abrir el archivo CSV
    if (($handle = fopen($csv, "r")) !== FALSE) {
        $headers = fgetcsv($handle, 1000, ";"); // Salta la cabecera
        while (($data = fgetcsv($handle, 1000, ";")) !== FALSE) {
            // Asigna cada columna a una variable (latitud, longitud, fecha, sensores...)
            list($lat, $lon, $year, $month, $day, $time, $temp, $pres, $hum, $pm2_5, $pm10) = $data;
            $timestamp = "$year-$month-$day $time"; // Construye la fecha completa con hora
            // Almacena la medición procesada como array asociativo (cast a float)
            $measurements[] = [
                'lat' => floatval($lat),
                'lon' => floatval($lon),
                'timestamp' => $timestamp,
                'temp' => floatval($temp),
                'pres' => floatval($pres),
                'hum' => floatval($hum),
                'pm2_5' => floatval($pm2_5),
                'pm10' => floatval($pm10)
            ];
        }
        fclose($handle);     // Cierra el archivo CSV al terminar
    }

    // Si hay menos de dos mediciones, error y termina
    if (count($measurements) < 2) {
        http_response_code(400);
        echo json_encode(['error' => 'Not enough measurements']);
        exit;
    }

    // Calcular estadísticas de la ruta: inicializa acumuladores
    $total_temp = $total_pm2_5 = $total_pm10 = $distance = 0;
    $count = count($measurements);

    // Recorre todas las mediciones para calcular sumas y distancia recorrida
    for ($i=0; $i<$count; $i++) {
        $m = $measurements[$i];
        $total_temp += $m['temp'];
        $total_pm2_5 += $m['pm2_5'];
        $total_pm10 += $m['pm10'];
        if ($i > 0) {
            // Calcula la distancia entre el punto anterior y el actual
            $p1 = [$measurements[$i-1]['lat'], $measurements[$i-1]['lon']];
            $p2 = [$m['lat'], $m['lon']];
            $distance += haversineGreatCircleDistance($p1[0], $p1[1], $p2[0], $p2[1]);
        }
    }
    // Calcula las medias
    $avg_temp = $total_temp / $count;
    $avg_pm2_5 = $total_pm2_5 / $count;
    $avg_pm10 = $total_pm10 / $count;
    // Calcula la duración total en segundos (diferencia entre primer y último timestamp)
    $duration = strtotime($measurements[$count-1]['timestamp']) - strtotime($measurements[0]['timestamp']);
    // Redondea la distancia recorrida a 2 decimales
    $distance_km = round($distance, 2);

    // Guardar en BD
    $db = db_connect();

    // Inserta la ruta principal en la tabla 'ride'
    $stmt = $db->prepare("INSERT INTO ride (avg_temp, avg_pm2_5, avg_pm10, duration, distance_km) VALUES (?, ?, ?, ?, ?)");
    $stmt->execute([$avg_temp, $avg_pm2_5, $avg_pm10, $duration, $distance_km]);
    $ride_id = $db->lastInsertId();

    // Inserta cada medición individual en la tabla 'measurement'
    $stmt2 = $db->prepare("INSERT INTO measurement (ride_id, timestamp, lat, lon, temp, pres, hum, pm2_5, pm10) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    foreach ($measurements as $m) {
        $stmt2->execute([
            $ride_id, $m['timestamp'], $m['lat'], $m['lon'], $m['temp'],
            $m['pres'], $m['hum'], $m['pm2_5'], $m['pm10']
        ]);
    }

    // Devuelve un JSON de éxito con el ID de la ruta
    echo json_encode(['message' => 'Ride inserted', 'ride_id' => $ride_id]);
}
?>