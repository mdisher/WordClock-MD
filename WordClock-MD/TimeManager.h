// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include <time.h>
#include "Config.h"

// =====================================================================
//  TimeManager
//  NTP time via configTzTime() using a POSIX TZ string, so DST is handled
//  automatically (no manual GMT/DST offsets). Fully non-blocking: it kicks
//  off SNTP and polls; the clock reads whatever the system RTC has. Once
//  seeded, the ESP32 RTC keeps time even if WiFi drops. Resyncs daily.
// =====================================================================
class TimeManager
{
public:
    TimeManager() : lastSyncMs(0), synced(false) {}

    // Start SNTP with the given POSIX TZ string. Non-blocking.
    void begin(const char *tzString);

    // Call every loop; triggers the daily resync.
    void update(uint32_t now);

    // Re-apply a (possibly new) timezone without waiting.
    void setTimezone(const char *tzString);

    // Force an NTP resync now (e.g. right after WiFi (re)connects).
    void forceResync();

    bool isTimeValid();               // RTC holds a plausible (post-2020) time
    bool hasSynced() const { return synced; } // achieved at least one valid read
    time_t nowEpoch();                // seconds since epoch (0 if invalid)
    struct tm getLocalTimeStruct();   // localtime; zeroed struct if invalid

private:
    uint32_t lastSyncMs;
    bool synced;
    char tz[48];

    void applyConfig();
};

#endif // TIME_MANAGER_H
