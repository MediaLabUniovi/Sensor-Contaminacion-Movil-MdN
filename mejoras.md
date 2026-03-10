# Registro de Mejoras del Firmware

En este documento se registran las modificaciones, ajustes y mejoras implementadas en el firmware del dispositivo.

## Mejoras Recientes

### Calibración y Ajuste del Lector de Batería (ADC)
*   **Problema:** El sistema estaba descalibrado, mostrando un 22.9% de batería con un voltaje real en celda de 3.643 V. El divisor de tensión y la referencia del ADC leían internamente ~3.275 V.
*   **Solución:** 
    *   Se calculó un nuevo factor de corrección para el ADC (1.1124).
    *   Se modificó el multiplicador `VOLTAGE_DIVIDER_RATIO` en el archivo `codigo/bici/src/config.h` de `2.0` al nuevo valor calibrado de `2.2249`.
    *   Se ajustó la curva de porcentaje en `codigo/bici/src/functions.cpp` para que marcara el 100% al llegar a los `4.15 V` (en lugar de `4.2 V`). La parte baja de la curva se mantiene en `0%` a `3.0 V`.
    *   Se modificó el comentario para reflejar que se usa una batería Li-ion 1S.

### Lógica de Suavizado de Lectura de Batería (Software)
*   **Problema:** A pesar de calcular la media de 20 muestras, el ruido eléctrico seguía causando ligeras fluctuaciones (por ejemplo, el porcentaje oscilaba momentáneamente un 1% arriba o abajo, creando saltos extraños en la interfaz de usuario).
*   **Solución:** Se implementó una lógica de un solo sentido para el porcentaje de batería en `codigo/bici/src/functions.cpp`.
    *   El programa recuerda el último porcentaje válido usando una variable `static`.
    *   Si la nueva lectura calculada es menor que el último porcentaje guardado, la asume como válida (la batería se descarga).
    *   Si la nueva lectura calculada es mayor, el código *la ignora*, asumiendo que es ruido del ADC. 
    *   Excepción: Si la nueva lectura calculada es mayor por una diferencia **superior al 3%**, sí la acepta como válida (asume que el usuario ha enchufado un cargador y la batería realmente se está cargando).

### Nombre del Dispositivo Bluetooth Dinámico
*   **Problema:** Todos los dispositivos anunciaban el mismo nombre estático (`AirQ-Sensor`), lo que creaba conflictos al tener múltiples prototipos encendidos a la vez. Además, en una primera versión, se utilizó la MAC del Wi-Fi, lo cual generaba discrepancias con la verdadera MAC del Bluetooth detectada por los escáneres nativos (Android).
*   **Solución:** Se modificó la inicialización del BLE en `codigo/bici/src/ble_manager.cpp`.
    *   Ahora, el microcontrolador lee específicamente la dirección MAC asignada al hardware Bluetooth a través de `esp_read_mac(..., ESP_MAC_BT)`.
    *   Extrae los 2 últimos bytes de esta verdadera MAC Bluetooth y los convierte a texto hexadecimal (equivale a los 4 últimos caracteres).
    *   Anuncia el nombre con el prefijo seguido de estas verdaderas letras/números finales (ej. `AirQ-A1B3`).

### Optimización de la Conexión GPS (Tiempos de Espera Neo-6M)
*   **Problema:** El módulo GPS tardaba mucho en obtener un Fix satelital (conexión en frío) o entregaba lecturas encoladas/perdía refresco. El intento de subir la velocidad a 115200 baudios causó un fallo total del módulo, por lo que se descartó mantener esa velocidad.
*   **Solución:** Se mantuvo el `GPS_BAUDRATE` a un valor estable de `9600` baudios, pero se aplicaron tres mejoras de temporización en `codigo/bici/src/main.cpp`:
    1.  **Tiempo de Cold-Start extendido:** Se amplió de 1000 ms a 5000 ms el tiempo de la primera ventana de escucha del GPS para asegurar que le da tiempo a enganchar las primeras tramas completas antes de saltar por timeout en arranques problemáticos.
    2.  **Reducción de latencia de Fix:** La validez de los datos posicionales se redujo de 2000 ms a 1500 ms; los datos mayores a este tiempo se descartarán, asegurando coordenadas más frescas.
    3.  **Aumento del buffer de lectura continua:** Se incrementó de 200 a 500 el límite de bytes leídos por ciclo en el Modo Continuo para permitir a TinyGPS vaciar el puerto serie en cada iteración de manera más eficiente.
