#include "DataLogger.h"

void recordData() {
    static unsigned long lastLogTime = 0;
    unsigned long currentMillis = millis();

    if (loggingActive && (currentMillis - lastLogTime >= LOG_INTERVAL_MS)) {
        lastLogTime = currentMillis;
        if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid() && gps.location.age() < 150) {
            digitalWrite(LED_BUILTIN, HIGH);
            int year = gps.date.year();
            byte month = gps.date.month();
            byte day = gps.date.day();
            byte hour = gps.time.hour();
            byte minute = gps.time.minute();
            byte second = gps.time.second();
            String timestamp = String(year) + "-" + padNumber(month) + "-" + padNumber(day) + " " +
                             padNumber(hour) + ":" + padNumber(minute) + ":" + padNumber(second);
            String csvLine = timestamp + "," + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
            logToFile(csvLine);
            logToSerial("New Line: " + csvLine);
        } else {
            logToSerial("No new data");
        }
    }
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