#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <SPI.h>
#include <SdFat.h>
#include <string.h>
#include <avr/wdt.h>
#include <TinyGPS++.h>

// Constants
const int CHIP_SELECT = 53;
const unsigned long LOG_INTERVAL_MS = 100;

// Function declarations
//     Commands.cpp
void StartCommand(const String &filename);
void StopCommand();
void ListCommand();
void ReadCommand(const String &filename);
void DeleteCommand(const String &filename);
void DebugCommand();
void GpsCommand();
void processSerialCommands();
void recordData();
void block();
void logToFile(const String &log);
void logToSerial(const String &log);
String padNumber(int num);

// External declarations
extern SdFat sd;
extern File logFile;
extern bool loggingActive;
extern bool debugMode;
extern TinyGPSPlus gps;

#endif