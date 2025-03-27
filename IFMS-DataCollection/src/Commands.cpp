#include "DataLogger.h"

void processSerialCommands() {
    if (!Serial.available()) {
        return;
    }

    String command = Serial.readStringUntil('\n');
    command.trim();

    // Split command into parts
    int spaceIndex = command.indexOf(' ');
    String cmd = spaceIndex == -1 ? command : command.substring(0, spaceIndex);
    String param = spaceIndex == -1 ? "" : command.substring(spaceIndex + 1);

    // Convert to uppercase for case-insensitive comparison
    cmd.toUpperCase();

    if (cmd == "START") {
        StartCommand(param);
    }
    else if (cmd == "STOP") {
        StopCommand();
    }
    else if (cmd == "LIST") {
        ListCommand();
    }
    else if (cmd == "READ") {
        ReadCommand(param);
    }
    else if (cmd == "DEBUG") {
        DebugCommand();
    }
    else if (cmd == "GPS") {
        GpsCommand();
    }
    else {
        logToSerial("Unknown command: " + command);
        logToSerial("Available commands: START <filename>, STOP, LIST, READ <filename>, DEBUG, GPS");
    }
}

void StartCommand(const String &filename) {
    if (logFile) {
      logFile.close();
    }
    bool fileExists = sd.exists(filename);
    logFile = sd.open(filename, O_WRITE | O_CREAT | O_APPEND);
    if (!logFile) {
      logToSerial("Error opening log file for writing.");
      block();
    }
    if (!fileExists) {
      logToFile("Timestamp,Lat,Lon");
      logToSerial("Adding CSV headers to new log file");
    }
    logToSerial("Logging started.");
    loggingActive = true;
  }
  
  void StopCommand() {
    if (logFile) {
      logFile.close();
    }
    logToSerial("Logging stopped.");  
    loggingActive = false;
  }
  
  void ListCommand() {
    File root = sd.open("/");
  
    if (!root) {
      logToSerial("Failed to open root directory.");
      return;
    }
  
    logToSerial("Listing files on SD Card:");
    while (true) {
      File entry = root.openNextFile();
      if (!entry) break;
  
      char name[256];
      entry.getName(name, sizeof(name));
      logToSerial(String(name));
      entry.close();
    }
    root.close();
  }
  
  void ReadCommand(const String &filename) {
    if (!sd.exists(filename)) {
      logToSerial("File does not exist: " + filename);
      return;
    }
  
    File file = sd.open(filename, O_READ);
    if (!file) {
      logToSerial("Error opening file for reading: " + filename);
      return;
    }
  
    logToSerial("Reading file: " + filename);
    while (file.available()) {
      String line = file.readStringUntil('\n');
      logToSerial(line);
    }
    file.close();
    logToSerial("File read complete.");
  }
  
  void DeleteCommand(const String &filename) {
    if (loggingActive && logFile) {
      logFile.close();
      loggingActive = false;
      logToSerial("Logging stopped to delete the file.");
    }
    if (!sd.exists(filename)) {
      logToSerial("File does not exist: " + filename);
      return;
    }
    if (sd.remove(filename)) {
      logToSerial("File deleted: " + filename);
    } else {
      logToSerial("Error deleting file: " + filename);
    }
  }
  
  void DebugCommand() {
    if (debugMode) {
      logToSerial("Disabling Debug Mode");
      debugMode = false;
    } else {
      debugMode = true;
      logToSerial("Debug Mode Enabled");
    }
  }
  
  void GpsCommand() {
    if (gps.location.isValid()) { 
      logToSerial("Latitude: " + String(gps.location.lat(), 6));
      logToSerial("Longitude: " + String(gps.location.lng(), 6));
      logToSerial("Age: " + String(gps.location.age()) + "ms");
    } else {
      logToSerial("No valid GPS fix.");
    }
  }