#include <SPI.h>
#include <SdFat.h>
#include <string.h>
#include <avr/wdt.h>

/*
Known Bugs:

- Start("file.csv"), Delete("file.csv"), Start("file.csv"), Delete("file.csv"). After creating the second file with the same name, delete keeps failing
*/

// LED Indicator
// Constant Red Light: Error has occurred and waiting for watchdog to restart the device
// Flashing Red Light: Logging is active
// One Flash After Command: Error processing command

// Constants
const int CHIP_SELECT = 53;
const unsigned long LOG_INTERVAL_MS = 100;

// Global Variables
SdFat sd;
File logFile;
bool loggingActive = false;
bool debugMode = true;

// Function Prototypes
//     Commands
void StartCommand(const String &filename);
void StopCommand();
void ListCommand();
void ReadCommand(const String &filename);
void DeleteCommand(const String &filename);
void DebugCommand();
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
    logToFile("MS");
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
    digitalWrite(LED_BUILTIN, HIGH);
    lastLogTime = currentMillis;
    String csvLine = String(currentMillis);
    logToFile(csvLine);
    logToSerial("New Line: " + csvLine);
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
}

void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  processSerialCommands();
  recordData();
  wdt_reset();
}