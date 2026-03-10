# Documentación del Firmware

## Funcionamiento del GPS

A continuación se detalla cómo está implementado el funcionamiento y el flujo de adquisición de datos del GPS en el sistema embebido. El comportamiento del GPS se gestiona desde `main.cpp`, dependiendo de la configuración en `config.h` y los comandos Bluetooth recibidos a través de `ble_manager.cpp`.

### 1. Modo Simulación

Durante el desarrollo y pruebas sin cobertura satelital, se puede utilizar el modo de simulación. Cuando en el archivo `config.h` se encuentra activa la directiva:
```cpp
#define SIMULATE_GPS
```
A efectos prácticos, **el sistema ignora completamente la lectura física a través del puerto Serie del módulo GPS**. 
En la fase de lectura (`DC_READ_GPS`), el código omite la comprobación del hardware, asumiendo que ha detectado posición. Posteriormente, en la fase de procesado (`DC_PROCESS_GPS`), inyecta **coordenadas fijas correspondientes a Gijón** (Lat: 43.53573, Lon: -5.66152) y un horario de sincronización estático.

*(Nota para producción: Para que el GPS real funcione, la línea `#define SIMULATE_GPS` debe estar comentada).*

---

### 2. Funcionamiento Físico Real (Sin `SIMULATE_GPS`)

El módulo GPS está conectado físicamente al microcontrolador ESP32 mediante el puerto Serie por hardware secundario (`Serial2`) operando a 9600 baudios. Su comportamiento de lectura **no siempre es continuo**; depende netamente de una configuración (el Modo GPS) que el usuario puede enviar por Bluetooth desde la sección *Settings* de la App móvil.

Existen dos flujos o metodologías de trabajo:

#### A) Modo Intervalo (`GPS_MODE_INTERVAL`) - *Modo por defecto*
Este modo está pensado para intentar optimizar el uso del procesador y la energía del ESP32.
1. Al llegar el momento de tomar una muestra (estado `DC_READ_GPS`), el microcontrolador entra en un bucle de lectura durante exactamente **1 segundo**.
2. Lee los paquetes del `Serial2` y se los pasa a la librería decodificadora `TinyGPS`. Si dentro de ese segundo logra armar una trama NMEA válida, guarda las coordenadas y actualiza el reloj (RTC) interno. Si pasa 1 segundo entero sin éxito, el sistema asume temporalmente que no hay cobertura.
3. Posteriormente, cuando el código entra en el estado `DC_WAIT` para esperar a la siguiente recolección (por ejemplo, 30 segundos de espera), **ignora completamente al GPS**. El dispositivo no lee el puerto serie satelital hasta que pasen esos 30 segundos y sea momento de medir otra vez.

#### B) Modo Continuo (`GPS_MODE_CONTINUOUS`)
Este modo se activa mandando el comando JSON correspondiente desde la App (`{"gpsMode": "continuous"}`). Es el modo idóneo para evitar que el buffer del puerto serie se atasque con datos antiguos si se quieren lecturas instantáneas.
1. La recolección de 1 segundo en `DC_READ_GPS` funciona de igual manera al modo de intervalo.
2. **Sin embargo**, durante los tiempos muertos de espera larga (estado `DC_WAIT`), el sistema hace esto continuamente en segundo plano:
   ```cpp
   if (currentGpsMode == GPS_MODE_CONTINUOUS) {
       while (Serial2.available()) {
           GPS.encode(Serial2.read()); // Alimentación constante del objeto de la librería
       }
   }
   ```
   Es decir, en este modo **el GPS sí se lee y decodifica de forma continua y en tiempo real** mientras el sistema principal "descansa", lo cual mantiene el objeto de la librería del GPS siempre actualizado y libre de buffer rebosado para cuando llegue el momento exacto de guardar el registro en la SD.

---

### 3. La dependencia vital: El Reloj Interno (RTC)

El sistema utiliza el GPS de forma dual: no solo para ubicarse geográficamente, sino como fuente de hora altamente precisa y fiable. Esta hora es vital para nombrar las columnas en el archivo `.csv` exportado.

* **Sincronización inicial:** Si el GPS obtiene cobertura (`newDataFlag = true`), el sistema extrae las coordenadas y utiliza la hora satelital para calibrar y ajustar su reloj interno (RTC - Real Time Clock) si no lo había hecho en la sesión actual.
* **Pérdida de señal:** Si el GPS pierde cobertura repentinamente (túneles, interiores, etc.), el sistema es consciente de ello. En vez de bloquear el programa o saltar de inmediato al estado de fallo LED rojo, confía en el calibrado de hora anterior. Utiliza su propio reloj interno (que sigue contando) para poner una marca de tiempo válida a esa última muestra de los sensores atmosféricos. Inscribe entonces un marcador `0.000000;0.000000` en las columnas de latitud y longitud, guardando los datos ambientales intactos. Adicionalmente, encenderá los LEDs pertinentes informando de la baja señal satelital, pero preservando la recolección de datos general.
