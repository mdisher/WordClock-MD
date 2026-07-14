#include "SerialHelper.h"

void initSerial()
{
#if USE_SERIAL
    Serial.begin(SERIAL_BAUD);
    // Give USB-CDC a moment to enumerate, but never block boot on it.
    uint32_t start = millis();
    while (!Serial && (millis() - start) < 800)
    {
        // non-blocking-ish: bounded wait, no delay() loop forever
    }
    SERIAL_PRINTLN();
    SERIAL_PRINTLN(F("[WordClock-MD] serial up"));
#endif
}
