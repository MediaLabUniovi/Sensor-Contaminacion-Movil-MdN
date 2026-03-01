#include "network_manager.h"
#include "functions.h" // Para blinking_led_sequence

bool connectToWiFi(Adafruit_NeoPixel &pixels) {
  pixels.clear();
  pixels.setPixelColor(0, COLOR_VERDE);
  pixels.setPixelColor(1, COLOR_VERDE);
  pixels.setPixelColor(2, COLOR_VERDE);
  pixels.show();

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  const int MAX_INTENTOS_WIFI = 3;
  int intentos = 0;

  // Intenta conectar automáticamente con las redes guardadas
  bool conectado = wm.autoConnect(AP_NAME, PASSWORD);
  while (!conectado && intentos < MAX_INTENTOS_WIFI) {
    Serial.printf("Intento de conexión fallido #%d\n", intentos + 1);
    delay(100);
    conectado = (WiFi.status() == WL_CONNECTED);
    intentos++;
    Serial.printf("Intentos: %d", intentos);
  }

  if (!conectado) {
    Serial.println("No se pudo conectar tras varios intentos. Borrando "
                   "configuración WiFi.");
    wm.resetSettings(); // borra SSID/password de la NVS
    delay(1000);
    blinking_led_sequence(COLOR_ROJO, 5, 500, pixels);
    return false;
  }

  Serial.println("Conectado correctamente a WiFi");
  return true;
}

bool uploadFileToServer(String filename, Adafruit_NeoPixel &pixels) {
  pixels.clear();
  pixels.setPixelColor(0, COLOR_VERDE);
  pixels.setPixelColor(1, COLOR_VERDE);
  pixels.setPixelColor(2, COLOR_VERDE);
  pixels.setPixelColor(3, COLOR_VERDE);
  pixels.show();

  if (filename == "") {
    Serial.println("No hay archivo target para subir.");
    return false;
  }

  Serial.println("Subiendo archivo: " + filename);
  File csvFile = SD.open(filename.c_str(), FILE_READ);
  if (!csvFile) {
    Serial.println("Error abriendo datos para envío: " + filename);
    return false;
  }
  size_t fileSize = csvFile.size();

  // Hosts and paths (hardcoded as in main.cpp)
  static const char host[] = "medialab-uniovi.es";
  static const uint16_t port = 443;
  static const char path[] = "/bike_pollution/upload.php";
  static const char boundary[] = "----WebKitFormBoundary7MA4YWxkTrZu0gW";

  char preamble[256];
  int plen = snprintf(preamble, sizeof(preamble),
                      "--%s\r\n"
                      "Content-Disposition: form-data; name=\"csv\"; "
                      "filename=\"datos.txt\"\r\n"
                      "Content-Type: text/csv\r\n\r\n",
                      boundary);

  if (plen < 0 || plen >= sizeof(preamble)) {
    Serial.println("Error construyendo preamble");
    csvFile.close();
    return false;
  }

  char ending[64];
  int elen = snprintf(ending, sizeof(ending), "\r\n--%s--\r\n", boundary);
  if (elen < 0 || elen >= sizeof(ending)) {
    Serial.println("Error construyendo ending");
    csvFile.close();
    return false;
  }

  size_t contentLength = (size_t)plen + fileSize + (size_t)elen;

  WiFiClientSecure client;
  client.setInsecure(); // acepta cualquier certificado
  Serial.printf("Conectando a %s:%u…\n", host, port);
  if (!client.connect(host, port)) {
    Serial.println("Error al conectar TLS");
    csvFile.close();
    return false;
  }

  // Headers
  client.print("POST ");
  client.print(path);
  client.print(" HTTP/1.1\r\n");
  client.print("Host: ");
  client.print(host);
  client.print("\r\n");
  client.print("User-Agent: ESP32\r\n");
  client.print("Connection: close\r\n");
  client.print("Content-Type: multipart/form-data; boundary=");
  client.print(boundary);
  client.print("\r\n");
  client.print("Content-Length: ");
  client.print(contentLength);
  client.print("\r\n\r\n");

  // Body
  client.write((const uint8_t *)preamble, plen);

  uint8_t buf[256];
  while (csvFile.available()) {
    size_t n = csvFile.read(buf, sizeof(buf));
    client.write(buf, n);
  }
  csvFile.close();

  client.write((const uint8_t *)ending, elen);

  // Response
  Serial.println("Esperando respuesta…");
  unsigned long timeout = millis() + 5000;
  while (!client.available() && millis() < timeout) {
    delay(10);
  }
  if (!client.available()) {
    Serial.println("Timeout leyendo respuesta");
    client.stop();
    return false;
  }

  while (client.available()) {
    String line = client.readStringUntil('\n');
    Serial.println(line);
  }

  client.stop();
  // Éxito
  return true;
}
