<?php
// Función para conectar a la base de datos MySQL usando PDO
function db_connect() {
    $host = 'localhost';        // Servidor de base de datos
    $db   = 'bike_pollution';   // Nombre de la base de datos
    $user = 'root';             // Usuario de la base de datos (por defecto en local suele ser 'root')
    $pass = '';                 // Contraseña del usuario
    $charset = 'utf8mb4';       // Juego de caracteres recomendado para soportar emojis y multilenguaje

    // DSN (Data Source Name): cadena de conexión para PDO
    $dsn = "mysql:host=$host;dbname=$db;charset=$charset";
    $options = [
        PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION, // Lanza excepciones si hay error en la BD
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,       // Devuelve los resultados como arrays asociativos
    ];
    try {
        // Intenta crear una nueva conexión PDO y la devuelve
         return new PDO($dsn, $user, $pass, $options);
    } catch (\PDOException $e) {
        // Si hay un error en la conexión, detiene el script y muestra un mensaje genérico
         exit('Database error');
    }
}
?>
