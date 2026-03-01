#include "sd_manager.h"
#include "mdc_contaminacion.hpp"

// Realiza la rotación de archivos data_0.csv ... data_9.csv
String performFileRotation() {
  int fileIndex = -1;
  for (int i = 0; i < 10; i++) {
    String fname = "/data_" + String(i) + ".csv";
    if (!SD.exists(fname)) {
      fileIndex = i;
      break;
    }
  }

  // Si están todos ocupados (0..9), rotamos.
  if (fileIndex == -1) {
    Serial.println("[SD] Memoria llena (10 archivos), rotando...");
    SD.remove("/data_0.csv");
    for (int i = 1; i < 10; i++) {
      String oldName = "/data_" + String(i) + ".csv";
      String newName = "/data_" + String(i - 1) + ".csv";
      SD.rename(oldName, newName);
    }
    fileIndex = 9; // El nuevo hueco es el último
  }

  String currentFilename = "/data_" + String(fileIndex) + ".csv";
  Serial.println("[SD] Nuevo archivo de sesión: " + currentFilename);
  return currentFilename;
}

// Genera un JSON con la lista de archivos { "files": ["/data_0.csv", ...] }
String listSDFilesJSON() {
  String fileList = "{\"files\":[";
  File root = SD.open("/");
  bool first = true;
  if (root) {
    File file = root.openNextFile();
    while (file) {
      if (!file.isDirectory()) {
        String name = String(file.name());
        // Filtramos solo los archivos data_X.csv
        // Nota: file.name() puede devolver con o sin '/' inicial dependiendo
        // del SDK
        if ((name.startsWith("/data_") || name.startsWith("data_")) &&
            name.endsWith(".csv")) {
          if (!first)
            fileList += ",";
          // Normalizamos siempre a /data_X.csv
          if (!name.startsWith("/")) {
            name = "/" + name;
          }
          fileList += "\"" + name + "\"";
          first = false;
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }
  fileList += "]}";
  return fileList;
}

// Borra un archivo específico
bool deleteSDFile(String filename) {
  if (SD.exists(filename)) {
    SD.remove(filename);
    return true;
  }
  return false;
}

void writeCSVHeader(String filename) {
  // Usamos writeFile de mdc_contaminacion.hpp (o FS)
  // writeFile(SD, filename.c_str(), "Latitud;Longitud;...");
  writeFile(SD, filename.c_str(),
            "Latitud;Longitud;Año;Mes;Día;hh:mm:ss;Temp;Pres;Hum;PM2.5;"
            "PM10;MAC\n");
}
