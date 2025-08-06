<?php
// Permite el acceso CORS para llamadas AJAX desde cualquier origen
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Headers: *");
header("Access-Control-Allow-Methods: GET, POST, OPTIONS");
// Maneja preflight CORS
if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit();
}

// Conecta a la base de datos
require_once 'db.php';
$db = db_connect();

// Si la petición viene con ?r=ID, devuelve los puntos de esa ruta
if (isset($_GET['r'])) {
    // Ejemplo: rides.php?r=3
    $ride_id = intval($_GET['r']);
    // Prepara la consulta para obtener todos los puntos GPS de la ruta
    $stmt = $db->prepare("SELECT lat, lon, timestamp, temp, pm2_5, pm10 FROM measurement WHERE ride_id = ? ORDER BY timestamp");
    $stmt->execute([$ride_id]);
    $rows = $stmt->fetchAll();
    // Devuelve los puntos como JSON
    header('Content-Type: application/json');
    echo json_encode($rows);
    exit;
}

// Si no viene ?r=ID, devuelve el listado de rutas recientes (últimos N días)
$days = isset($_GET['days']) ? intval($_GET['days']) : 30;
// Calcula la fecha mínima para filtrar rutas recientes
$since_date = date('Y-m-d H:i:s', strtotime("-$days days"));
// Prepara la consulta para obtener las rutas
$stmt = $db->prepare("SELECT id, created_at, avg_temp, avg_pm2_5, avg_pm10, duration, distance_km FROM ride WHERE created_at >= ? ORDER BY created_at DESC");
$stmt->execute([$since_date]);
$rides = $stmt->fetchAll();
// Devuelve el listado como JSON
header('Content-Type: application/json');
echo json_encode($rides);
?>
