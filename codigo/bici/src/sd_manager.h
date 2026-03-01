#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include "Arduino.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"

// Realiza la rotación de archivos data_0.csv ... data_9.csv
// Retorna el nombre del nuevo archivo creado (e.g., "/data_9.csv")
String performFileRotation();

// Genera un JSON con la lista de archivos { "files": ["/data_0.csv", ...] }
String listSDFilesJSON();

// Borra un archivo específico
bool deleteSDFile(String filename);

// Escribe la cabecera CSV en el archivo especificado
void writeCSVHeader(String filename);

#endif // SD_MANAGER_H
