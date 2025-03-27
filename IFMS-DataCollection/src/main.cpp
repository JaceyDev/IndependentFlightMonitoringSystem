#include "DataLogger.h"

// Global variable definitions
SdFat sd;
File logFile;
bool loggingActive = false;
bool debugMode = false;
TinyGPSPlus gps;

void setup() {
    wdt_disable();
    Serial.begin(9600);
    Serial1.begin(9600);
    while (!Serial) {
        // Wait for serial port to connect
    }
    
    wdt_enable(WDTO_8S);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(CHIP_SELECT, OUTPUT);

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
    }
    wdt_reset();
}