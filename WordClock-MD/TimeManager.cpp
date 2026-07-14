// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#include "TimeManager.h"
#include "SerialHelper.h"

// Any epoch past 2020-01-01 means SNTP has seeded the RTC.
#define VALID_EPOCH_THRESHOLD 1577836800UL

void TimeManager::applyConfig()
{
    // Sets TZ (with DST rules) and starts SNTP against the pool. Returns
    // immediately; the RTC updates in the background when a packet arrives.
    configTzTime(tz, NTP_SERVER_1, NTP_SERVER_2);
    lastSyncMs = millis();
}

void TimeManager::begin(const char *tzString)
{
    strncpy(tz, tzString ? tzString : DEFAULT_TZ, sizeof(tz) - 1);
    tz[sizeof(tz) - 1] = '\0';
    applyConfig();
    SERIAL_PRINTF("[Time] SNTP started, TZ=%s\n", tz);
}

void TimeManager::setTimezone(const char *tzString)
{
    if (!tzString)
        return;
    strncpy(tz, tzString, sizeof(tz) - 1);
    tz[sizeof(tz) - 1] = '\0';
    // Re-apply TZ immediately so the displayed time shifts without a resync,
    // and re-arm SNTP.
    setenv("TZ", tz, 1);
    tzset();
    applyConfig();
    SERIAL_PRINTF("[Time] TZ changed -> %s\n", tz);
}

void TimeManager::forceResync()
{
    applyConfig();
    SERIAL_PRINTLN(F("[Time] forced NTP resync"));
}

void TimeManager::update(uint32_t now)
{
    if (now - lastSyncMs >= NTP_RESYNC_INTERVAL_MS)
    {
        SERIAL_PRINTLN(F("[Time] daily resync"));
        applyConfig();
    }
}

bool TimeManager::isTimeValid()
{
    time_t t = time(nullptr);
    bool ok = (t > (time_t)VALID_EPOCH_THRESHOLD);
    if (ok && !synced)
    {
        synced = true;
        SERIAL_PRINTLN(F("[Time] first valid time acquired"));
    }
    return ok;
}

time_t TimeManager::nowEpoch()
{
    return isTimeValid() ? time(nullptr) : (time_t)0;
}

struct tm TimeManager::getLocalTimeStruct()
{
    struct tm info;
    memset(&info, 0, sizeof(info));
    if (isTimeValid())
    {
        time_t t = time(nullptr);
        localtime_r(&t, &info);
    }
    return info;
}
