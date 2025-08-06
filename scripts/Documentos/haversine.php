<?php
// Función para calcular la distancia en línea recta entre dos puntos geográficos
function haversineGreatCircleDistance($latitudeFrom, $longitudeFrom, $latitudeTo, $longitudeTo, $earthRadius = 6371)
{
  // Convierte las coordenadas de grados a radianes
  $latFrom = deg2rad($latitudeFrom);
  $lonFrom = deg2rad($longitudeFrom);
  $latTo = deg2rad($latitudeTo);
  $lonTo = deg2rad($longitudeTo);

  // Diferencia de latitud y longitud en radianes
  $latDelta = $latTo - $latFrom;
  $lonDelta = $lonTo - $lonFrom;

  // Aplicación de la fórmula de haversine:
  // a es el cuadrado del semiverseno de la distancia angular entre los dos puntos
  $a = sin($latDelta / 2) * sin($latDelta / 2) +
       cos($latFrom) * cos($latTo) *
       sin($lonDelta / 2) * sin($lonDelta / 2);
  // c es el ángulo central en radianes
  $c = 2 * atan2(sqrt($a), sqrt(1 - $a));

  // Devuelve la distancia multiplicando el radio de la Tierra por el ángulo central
  return $earthRadius * $c;
}
?>
