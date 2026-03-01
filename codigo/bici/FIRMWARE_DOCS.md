# Documentación del Firmware - PolutionBike v2

Esta documentación detalla la estructura, módulos y flujo de ejecución del firmware del sensor de contaminación (ESP32).

## 1. Arquitectura del Proyecto

El proyecto sigue una arquitectura modular basada en **Managers**, controlados por una máquina de estados central en `main.cpp`.

### Estructura de Directorios (`src/`)
*   **`main.cpp`**: Punto de entrada. Contiene `setup()`, `loop()` y la máquina de estados principal.
*   **`config.h`**: Archivo de configuración global (Pines, UUIDs BLE, Constantes).
*   **`ble_manager.cpp/.h`**: Encapsula toda la lógica Bluetooth Low Energy.
*   **`sd_manager.cpp/.h`**: Abstrae las operaciones de sistema de archivos (SD).
*   **`network_manager.cpp/.h`**: Gestiona la conectividad WiFi y subida de datos HTTP.
*   **`mdc_contaminacion.hpp`**: Funciones auxiliares heredadas (Lectura de sensores, utilidades LED).

---

## 2. Máquina de Estados (`main.cpp`)

El sistema opera en un bucle infinito (`loop`) que evalúa la variable `currentMode`.

| Estado | Descripción | Transición |
| :--- | :--- | :--- |
| **`IDLE`** | Estado de reposo. LED Verde (o Azul si BLE activo). Espera comando BLE o pulsación. | -> `DATA_RECOLLECTION` (Pulsación Larga) <br> -> `WIFI_CONNECTION` (Comando BLE) |
| **`PROCESS_LONG_PRESS`** | Evalúa pulsación de botón. | -> `DATA_RECOLLECTION` (Inicio Ruta) <br> -> `IDLE` (Cancelado) <br> -> `WIFI_CONNECTION` (Fin Ruta) |
| **`DATA_RECOLLECTION`** | Bucle de lectura de sensores. Graba en SD y notifica por BLE. | -> `PROCESS_LONG_PRESS` (Interrupción Usuario) |
| **`WIFI_CONNECTION`** | Intenta conectar a WiFi guardada o inicia Portal Captivo. | -> `SEND_DATA` (Éxito) <br> -> `IDLE` (Fallo) |
| **`SEND_DATA`** | Sube el archivo `uploadTarget` al servidor. | -> `IDLE` (Reinicio post-upload) |

---

## 3. Módulos y API

### 3.1 BLE Manager (`ble_manager.h`)
Encargado de la comunicación con la App.
*   `void initBLE()`: Inicializa stack BLE, servidor y características.
*   `void startBLEAdvertising()` / `stopBLEAdvertising()`: Controla visibilidad.
*   `void notifySensorData(...)`: Envía paquete de datos (Temp, Hum, PM, GPS) a la App.
*   `void notifyGPSStatus(String status)`: Informa a la App si busca GPS ("searching_gps").
*   **Callbacks**: Procesa JSONs de escritura como `{"measureInterval": 10}` o `{"cmd": "uploadFile"}`.

### 3.2 SD Manager (`sd_manager.h`)
Encargado de la persistencia de datos.
*   `String performFileRotation()`: Busca el siguiente archivo libre (`data_0`..`data_9`). Si está lleno, rota (borra el 0 y desplaza). **Se llama solo al inicio de `PROCESS_LONG_PRESS`**.
*   `String listSDFilesJSON()`: Escanea el directorio raíz y devuelve JSON `{"files": ["/data_9.csv"]}`.
*   `bool deleteSDFile(String filename)`: Elimina un archivo específico.
*   `void writeCSVHeader(String filename)`: Escribe la cabecera de columnas en un archivo nuevo.

### 3.3 Network Manager (`network_manager.h`)
Encargado de la conectividad nube.
*   `bool connectToWiFi(Adafruit_NeoPixel &pixels)`: Intenta conexión. Si falla, levanta un AP "Sensor_Setup" para configurar credenciales.
*   `bool uploadFileToServer(String filename, ...)`: Lee el archivo de la SD y lo envía vía POST Multipart al servidor configurado en `config.h`.

---

## 4. Flujo de Datos

1.  **Inicio Sesión**: Usuario pulsa botón (2s).
2.  **Creación Archivo**: `main.cpp` llama a `sd_manager::performFileRotation()`. Se crea `/data_N.csv`.
3.  **Bucle de Medida** (`DATA_RECOLLECTION`):
    *   Lee GPS (o simula).
    *   Lee BME280 y SPS30.
    *   **Escribe en SD**: Añade línea CSV a `/data_N.csv`.
    *   **Notifica BLE**: Si `bluetoothEnabled` es true, envía datos a la App.
    *   Espera `TIEMPO_ENTRE_MEDIDAS`.
4.  **Fin Sesión**: Usuario pulsa botón (2s).
5.  **Subida (Opcional)**: Usuario o Flujo automático activa `WIFI_CONNECTION`.
    *   Sensor sube `/data_N.csv`.
    *   Sensor se reinicia.
