#include "DataLogger.h"

String padNumber(int num) {
    return num < 10 ? "0" + String(num) : String(num);
}

void block() {
    digitalWrite(LED_BUILTIN, HIGH);
    while (1);
}