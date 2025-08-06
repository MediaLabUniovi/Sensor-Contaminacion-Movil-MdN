# Sensor-Contaminacion-Movil-MdN
Sensor de contaminación móvil para el reto TICLab 2025 Mar de Niebla
Este proyecto consiste en un sensor de contaminación y de temperatura, presión y humedad para ser colocado en bicicletas y tener un registro en mayor cantidad de puntos de estos parámetros.

## Funcionamiento
El sensor consiste en una carcasa con 5 LEDs y un botón de usuario.
Al encender el dispositivo, aparecerá un LED encendido en verde, si la inicialización de todos los componentes es correcta, entonces se encederá un segundo LED.

Si se pulsa el botón durante 3 segundos, entonces aparecerá un tercer LED en verde, eso significa que el sensor empieza a recolectar datos. Aquí puede aparecer un secuecnia de luces rojas cada cierto tiempo (indicando que no hay señal GPS y por tanto, no se están recolectando datos) o 3 LEDs que indican el estado de las medidas según la siguiente tabla:

| LED (pixel index) | Función           | Condición                                 | Color      |
|-------------------|-------------------|-------------------------------------------|------------|
| **0**             | PM₂.₅             | ≤ 12.0 µg/m³                              | Verde      |
|                   |                   | 12.1 – 35.4 µg/m³                         | Naranja    |
|                   |                   | > 35.4 µg/m³                              | Rojo       |
| **1**             | PM₁₀              | ≤ 54.0 µg/m³                              | Verde      |
|                   |                   | 54.1 – 154.0 µg/m³                        | Naranja    |
|                   |                   | > 154.0 µg/m³                             | Rojo       |
| **2**             | Estado del sistema| `ret < 0` (error SPS30)                   | Rojo       |
|                   |                   | `!bme_ok` (BME280 no inicializado)        | Naranja    |
|                   |                   | `!SD_ok` (SD no accesible)                | Amarillo   |
|                   |                   | — ninguna de las anteriores (todo OK)     | Azul       |
| **0–4**           | Sin señal GPS     | `newDataFlag == false`                    | Parpadeo Rojo (todos) |


Si después de hacer estas medidas se vuelve a mantener el botón durante 3 segundos entonces pasamos al modo de conexión WiFi (esto se indicará con un cuarto LED en verde). El sensor buscará si tiene una red WiFi ya guardada a la que conectarse y si no iniciará su proprio punto de acceso con nombre SensorMovil y contraseña RetoTicLab. Deberemos conectarnos a ese punto de acceso y meter las credenciales de red a la que queramos que se conecte nuestro dispositivo. Una vez hecho, si la conexión resulta correcta, se nos indicará que está mandando datos con un quinto LED en verde.

Una vez que se hayan mandado todos los datos, habrá una secuencia de parpadeo en verde y el sistema volverá al principio. Los datos de la tarjeta SD se borrarán al pasar del modo standby al de recolección de datos.
