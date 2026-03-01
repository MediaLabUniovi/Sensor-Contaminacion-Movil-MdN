# Sensor-Contaminacion-Movil-MdN
Sensor de contaminación móvil para el reto TICLab 2025 Mar de Niebla
Este proyecto consiste en un sensor de contaminación y de temperatura, presión y humedad para ser colocado en bicicletas y tener un registro en mayor cantidad de puntos de estos parámetros.

## Funcionamiento
El sensor consiste en una carcasa con 5 LEDs y un botón de usuario.

### Modos de Operación y Controles
Al encender el dispositivo, el sistema arranca en estado de reposo (IDLE). En este estado de espera (antes de empezar a recolectar datos), puedes elegir el modo de funcionamiento con un **click simple** (pulsación corta) del botón:
- **Modo Normal (LED 0 en Verde)**: Funciona de manera completamente autónoma sin la aplicación móvil. Al iniciar la ruta, se apaga el Bluetooth para ahorrar batería y los datos se guardan únicamente en la tarjeta SD local.
- **Modo Bluetooth (LED 0 en Azul)**: Funciona junto con la aplicación móvil. El dispositivo mantiene activado el Bluetooth y manda los datos leídos a la App en tiempo real, además de guardarlos en la tarjeta SD.

### Recolección de Datos
Si **mantienes el botón pulsado por 2 segundos** (pulsación larga), el sensor inicia la recolección de datos. Al principio de la lectura de sensores, el LED 0 se encenderá en **Naranja** a modo de confirmación o "lectura en proceso". Tras tomar todas las medidas, los 5 LEDs indicarán el estado del sistema según esta tabla:

| LED (index) | Función                | Condición                                      | Color      |
|-------------|------------------------|------------------------------------------------|------------|
| **0**       | Estado del Sistema     | Todos los componentes OK (`bme_ok`, `SD_ok`, `SPS30`) | Verde      |
|             |                        | Algún fallo drástico en los componentes        | Rojo       |
| **1**       | Batería                | > 60%                                          | Verde      |
|             |                        | 20% - 60%                                      | Amarillo   |
|             |                        | < 20%                                          | Rojo       |
| **2**       | GPS                    | Simulación (`SIMULATE_GPS` activa)             | Cian       |
|             |                        | Señal correcta satelital (GPS fix reciente)    | Verde      |
|             |                        | Señal perdida pero reloj interno válido (RTC)  | Amarillo   |
|             |                        | Sin señal y sin reloj interno válido           | Rojo       |
| **3**       | PM₂.₅                  | ≤ 12.0 µg/m³                                   | Verde      |
|             |                        | 12.1 – 35.4 µg/m³                              | Naranja    |
|             |                        | > 35.4 µg/m³                                   | Rojo       |
| **4**       | PM₁₀                   | ≤ 54.0 µg/m³                                   | Verde      |
|             |                        | 54.1 – 154.0 µg/m³                             | Naranja    |
|             |                        | > 154.0 µg/m³                                  | Rojo       |

### Fin de Ruta y Transmisión de Datos
Si vuelves a realizar una **pulsación larga** (2 segundos) para finalizar la recolección, el comportamiento dependerá del modo elegido al principio:
- **En Modo Normal**: Al finalizar, el sistema pasará a modo de **Conexión WiFi** (secuencia Verde/Rojo). El dispositivo buscará una red WiFi guardada o creará su propio punto de acceso (nombre de red: `SensorMovil`, contraseña: `RetoTicLab`). Tras conectarte a la red y configurar sus credenciales, el sistema subirá el último archivo de datos a la base de datos y se reiniciará automáticamente (secuencia en verde al terminar).
- **En Modo Bluetooth**: Al finalizar, el sistema **volverá a su estado inicial de reposo (IDLE)** automáticamente en vez de encender la WiFi. En este modo, el usuario puede extraer los ficheros utilizando la App móvil por Bluetooth para subirlos posteriormente.

*Aclaración:* Ya no se borran los archivos antiguos cada vez que se enciende el dispositivo; el sistema genera un fichero nuevo consecutivo para cada sesión de medición diferente.

## Uso
Actualmente el proyecto está hecho en VSCode con PlatformIO. Para una mejor lecutra de los comentarios se recomienda utilizar la extensión better comments con el Json de configuración que aparece en la carpeta codigo.
