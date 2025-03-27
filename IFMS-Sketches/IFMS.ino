#include <SPI.h>
#include <SdFat.h>
#include <string.h>
#include <avr/wdt.h>
#include <TinyGPS++.h> 

/*
Known Bugs:
- Start("file.csv"), Delete("file.csv"), Start("file.csv"), Delete("file.csv"). After creating the second file with the same name, delete keeps failing
*/

// LED Indicator
// Constant Red Light: Error has occurred and waiting for watchdog to restart the device
// Flashing Red Light: GPS Signal is valid and has been logged. Infrequent flashing means the gps data is stale / lack of connection
// One Flash After Command: Error processing command

// Constants
const int CHIP_SELECT = 53;
const unsigned long LOG_INTERVAL_MS = 100;

// Global Variables
SdFat sd;
File logFile;
bool loggingActive = false;
bool debugMode = false;

TinyGPSPlus gps;

// Function Prototypes
//     Commands
void StartCommand(const String &filename);
void StopCommand();
void ListCommand();
void ReadCommand(const String &filename);
void DeleteCommand(const String &filename);
void DebugCommand();
void GpsCommand();
//     Serial
void processSerialCommands();
//     General
void recordData();
void block();
void logToFile(const String &log);
void logToSerial(const String &log);

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

void processSerialCommands() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    logToSerial("Command received: " + input);

    int spaceIndex = input.indexOf(' ');
    String command;
    String arg1 = "";

    if (spaceIndex == -1) {
      command = input;
    } else {
      command = input.substring(0, spaceIndex);
      arg1 = input.substring(spaceIndex + 1); 
      arg1.trim();
    }
    command.toLowerCase();

    if (command == "start") {
      if (arg1 == "") {
        logToSerial("Error: START command requires a filename.");
        digitalWrite(LED_BUILTIN, HIGH);
      } else {
        StartCommand(arg1);
      }
    } else if (command == "stop") {
      StopCommand();
    } else if (command == "list") {
      ListCommand();
    } else if (command == "read") {
      if (arg1 == "") {
        logToSerial("Error: READ command requires a filename.");
        digitalWrite(LED_BUILTIN, HIGH);
      } else {
        ReadCommand(arg1);
      }
    } else if (command == "delete") {
      if (arg1 == "") {
        logToSerial("Error: DELETE command requires a filename.");
        digitalWrite(LED_BUILTIN, HIGH);
      } else {
        DeleteCommand(arg1);
      }
    } else if (command == "debug") {
      DebugCommand();
    } else if (command == "gps") {
      GpsCommand();
    } else {
      logToSerial("Unknown command. Use 'START {FILENAME}', 'STOP', 'LIST', 'READ {FILENAME}', 'DELETE {FILENAME}', 'DEBUG'");
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
}

void recordData() {
  static unsigned long lastLogTime = 0;
  unsigned long currentMillis = millis();

  if (loggingActive && (currentMillis - lastLogTime >= LOG_INTERVAL_MS)) {
    lastLogTime = currentMillis;
    // GPS data will only get logged if the date, time and location is valid and the location data is not older than 150ms
    // 150ms is an arbitary number, subject to change. Just dont want stale data being logged
    if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid() && gps.location.age() < 150) {  // Check if all required data is valid
      digitalWrite(LED_BUILTIN, HIGH);
      // Extract GPS date and time
      int year = gps.date.year();
      byte month = gps.date.month();
      byte day = gps.date.day();
      byte hour = gps.time.hour();
      byte minute = gps.time.minute();
      byte second = gps.time.second();
      // YYYY-MM-DD HH:MM:SS
      String timestamp = String(year) + "-" + padNumber(month) + "-" + padNumber(day) + " " +
                         padNumber(hour) + ":" + padNumber(minute) + ":" + padNumber(second);
      // Construct the CSV line with the timestamp
      String csvLine = timestamp + "," + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
      logToFile(csvLine);
      logToSerial("New Line: " + csvLine);
    } else {
      logToSerial("No new data");
    }
  }
}

void block() {
  digitalWrite(LED_BUILTIN, HIGH);
  while (1);
}

void logToFile(const String &log) {
  logFile.println(log);
  logFile.flush();
}

void logToSerial(const String &log) {
  if (debugMode) {
    Serial.println(log);
  }
}

void setup() {
  wdt_disable();

  Serial.begin(9600);
  Serial1.begin(9600);
  while (!Serial) {
    // Wait for serial port to connect. Temporary.
  }

  // Enable watchdog with 8s timer
  wdt_enable(WDTO_8S);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(CHIP_SELECT, OUTPUT);

  // Wait SD Card connection
  logToSerial("Initialising SD card...");
  if (!sd.begin(CHIP_SELECT)) {
    logToSerial("SD card initialization failed! Check wiring and card insertion.");
    block();
  }
  logToSerial("SD card initialized.");
  wdt_reset();

  logToSerial("Send 'START {FILENAME}' to begin logging, 'STOP' to end logging,");
  logToSerial("'LIST' to list files, 'READ {FILENAME}' to read a file.");

  StartCommand(String(random(1000))+".csv");
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  processSerialCommands();
  recordData();
  while (Serial1.available()) {
    char c = Serial1.read();
    gps.encode(c); 
    //logToSerial(c);
  }
  wdt_reset();
}

String padNumber(int num) {
  return num < 10 ? "0" + String(num) : String(num);
}