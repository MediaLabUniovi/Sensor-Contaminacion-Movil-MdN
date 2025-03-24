// Fichero con funciones de gestión de archivos 
#ifndef SD_INCLUDES
#define SD_INCLUDES
#include "FS.h"     // ESP32 FileSystem (https://github.com/espressif/arduino-esp32/tree/master/libraries/FS)
#include "SD.h"     // ESP32 SD (https://github.com/espressif/arduino-esp32/tree/master/libraries/SD)
#include "SPI.h"    // ESP32 SPI (https://github.com/espressif/arduino-esp32/tree/master/libraries/SPI)
#endif  // SD_INCLUDES
/*
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | SPI Pin Name | ESP8266 | ESP32 | ESP32‑S2 | ESP32‑S3 | ESP32‑C3 | ESP32‑C6 | ESP32‑H2 |
 * +==============+=========+=======+==========+==========+==========+==========+==========+
 * | CS (SS)      | GPIO15  | GPIO5 | GPIO34   | GPIO10   | GPIO7    | GPIO18   | GPIO0    |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | DI (MOSI)    | GPIO13  | GPIO23| GPIO35   | GPIO11   | GPIO6    | GPIO19   | GPIO25   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | DO (MISO)    | GPIO12  | GPIO19| GPIO37   | GPIO13   | GPIO5    | GPIO20   | GPIO11   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * | CLK_PIN(SCLK)| GPIO14  | GPIO18| GPIO36   | GPIO12   | GPIO4    | GPIO21   | GPIO10   |
 * +--------------+---------+-------+----------+----------+----------+----------+----------+
 * https://github.com/espressif/arduino-esp32/tree/master/libraries/SD
 */

//listDir: Imprime en pantalla un listado del directorio objetivo
void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
        Serial.print("  DIR : ");
        Serial.println(file.name());
        if (levels) {
            listDir(fs, file.path(), levels - 1);
        }
        } else {
        Serial.print("  FILE: ");
        Serial.print(file.name());
        Serial.print("  SIZE: ");
        Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}
//createDir: crea un directorio con el path indicado
void createDir(fs::FS &fs, const char *path) {
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}
//removeDir: borra un directorio con el path indicado
void removeDir(fs::FS &fs, const char *path) {
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}
//readFile: imprime en el terminal los contenidos de un archivo con el path indicado
void readFile(fs::FS &fs, const char *path) {
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}
//writeFile: escribe una cadena de texto al principio del archivo en el path indicado
void writeFile(fs::FS &fs, const char *path, const char *message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}
//appendFile: escribe una cadena de texto al final del archivo en el path indicado
void appendFile(fs::FS &fs, const char *path, const char *message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)) {
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}
//renameFile: renombra un archivo con un path nuevo
void renameFile(fs::FS &fs, const char *path1, const char *path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if(fs.rename(path1, path2)){
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}
//deleteFile: borra el archivo en el path indicado.
void deleteFile(fs::FS &fs, const char *path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}
//testFileIO: 
void testFileIO(fs::FS &fs, const char *path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len) {
        size_t toRead = len;
        if (toRead > 512) {
            toRead = 512;
        }
        file.read(buf, toRead);
        len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %lu ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }

    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for (i = 0; i < 2048; i++) {
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %lu ms\n", 2048 * 512, end);
    file.close();
}