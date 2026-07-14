#ifndef WORDCLOCK_H
#define WORDCLOCK_H

#include <Arduino.h>
#include "ClockDisplayHAL.h"
#include "ColorEngine.h"
#include "TimeManager.h"

// =====================================================================
//  WordClock
//  Time -> words logic (ported from the original, minus the blocking bits).
//  compose() paints the current time into the display buffer every frame;
//  the caller (the .ino) applies any status overlay and calls hal.show().
//  Colors come from ColorEngine, so RANDOM re-rolls only on the minute
//  (no per-second flicker) while gradient modes animate smoothly.
// =====================================================================
class WordClock
{
public:
    WordClock(ClockDisplayHAL *hal, ColorEngine *color, TimeManager *time);

    void begin();

    // Paint the current time into the display buffer (does not call show()).
    void compose(uint32_t now);

    // True exactly once when a new hour begins (for the on-the-hour animation).
    bool consumeHourTrigger();

private:
    ClockDisplayHAL *hal;
    ColorEngine *color;
    TimeManager *time;

    int lastMinute;
    int lastHourTriggered;
    bool firstCompose;
    bool hourTriggerPending;

    String getMinutesWord(int minute) const;
    void lightWord(const String &word, uint16_t slot, uint32_t now);
};

#endif // WORDCLOCK_H
