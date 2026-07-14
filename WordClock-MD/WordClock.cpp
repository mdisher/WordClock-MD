// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#include "WordClock.h"
#include "SerialHelper.h"

WordClock::WordClock(ClockDisplayHAL *hal, ColorEngine *color, TimeManager *time)
    : hal(hal), color(color), time(time),
      lastMinute(-1), lastHourTriggered(-1),
      firstCompose(true), hourTriggerPending(false) {}

void WordClock::begin()
{
    lastMinute = -1;
    lastHourTriggered = -1;
    firstCompose = true;
    hourTriggerPending = false;
}

// Same 5-minute buckets as the original (matches the printed template).
String WordClock::getMinutesWord(int minute) const
{
    if (minute < 5)
        return "OCLOCK";
    else if (minute < 10)
        return "FIVE";
    else if (minute < 15)
        return "TEN";
    else if (minute < 20)
        return "FIFTEEN";
    else if (minute < 25)
        return "TWENTY";
    else if (minute < 30)
        return "TWENTYFIVE";
    else if (minute < 35)
        return "THIRTY";
    else if (minute < 40)
        return "TWENTYFIVE";
    else if (minute < 45)
        return "TWENTY";
    else if (minute < 50)
        return "FIFTEEN";
    else if (minute < 55)
        return "TEN";
    else
        return "FIVE";
}

// Light one word either as a flat slot color or, in spatial gradient mode,
// per-LED across its footprint.
void WordClock::lightWord(const String &word, uint16_t slot, uint32_t now)
{
    if (color->isSpatial())
    {
        uint8_t s, e;
        if (hal->wordRange(word, s, e))
        {
            for (uint8_t idx = s; idx <= e; ++idx)
            {
                uint8_t x, y;
                hal->indexToXY(idx, x, y);
                hal->setIndex(idx, color->colorForXY(x, y, now));
            }
        }
    }
    else
    {
        hal->displayWord(word, color->colorForSlot(slot, now));
    }
}

void WordClock::compose(uint32_t now)
{
    struct tm t = time->getLocalTimeStruct();
    int hour = t.tm_hour % 12;
    if (hour == 0)
        hour = 12;
    int minute = t.tm_min;

    // Re-roll random colors only when the minute changes.
    if (minute != lastMinute)
    {
        color->newMinute();
        lastMinute = minute;
    }

    // Fire the on-the-hour trigger once per new hour.
    if (minute == 0)
    {
        if (!firstCompose && t.tm_hour != lastHourTriggered)
            hourTriggerPending = true;
        lastHourTriggered = t.tm_hour;
    }
    firstCompose = false;

    hal->clear();

    // Phrasing, built in reading order: "IT IS <minutes> PAST|TO <hour>", or
    // "IT IS <hour> O'CLOCK" on the top of the hour. `slot` numbers each lit
    // word 0,1,2,... so ColorEngine can hand every word its own color.
    uint16_t slot = 0;
    lightWord("IT", slot++, now);
    lightWord("IS", slot++, now);

    if (minute < 5)
    {
        // :00-:04 -> "<hour> O'CLOCK" (hour stays as-is).
        lightWord("OCLOCK", slot++, now);
    }
    else if (minute < 35)
    {
        // :05-:34 -> "<minutes> PAST <hour>".
        lightWord("PAST", slot++, now);
        lightWord("MINUTES", slot++, now);
    }
    else
    {
        // :35-:59 -> "<minutes> TO <next hour>", so roll the hour forward one
        // (wrapping 12 -> 1). getMinutesWord() already mirrors the bucket
        // (e.g. :40 -> TWENTY) to read as "TWENTY MINUTES TO".
        lightWord("TO", slot++, now);
        lightWord("MINUTES", slot++, now);
        hour = (hour + 1) % 12;
        if (hour == 0)
            hour = 12;
    }

    lightWord(getMinutesWord(minute), slot++, now);
    lightWord(String("HOUR_") + String(hour), slot++, now);
}

bool WordClock::consumeHourTrigger()
{
    bool t = hourTriggerPending;
    hourTriggerPending = false;
    return t;
}
