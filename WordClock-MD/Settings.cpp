// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#include "Settings.h"
#include "SerialHelper.h"
#include <Preferences.h>

#define SETTINGS_MAGIC 0x5743   // 'W','C'
#define SETTINGS_VERSION 1
#define NVS_NAMESPACE "wordclock"
#define NVS_BLOB_KEY "cfg"

static Preferences prefs;

// Safe bounded copy into a fixed char[] (always NUL-terminated).
static void copyStr(char *dst, size_t dstSize, const char *src)
{
    if (!dst || dstSize == 0)
        return;
    if (!src)
    {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

void Settings::applyDefaults()
{
    memset(&data, 0, sizeof(data));
    data.magic = SETTINGS_MAGIC;
    data.version = SETTINGS_VERSION;
    data.ssid[0] = '\0';
    data.pass[0] = '\0';
    copyStr(data.tz, sizeof(data.tz), DEFAULT_TZ);
    copyStr(data.hostname, sizeof(data.hostname), DEFAULT_HOSTNAME);
    data.brightness = DEFAULT_BRIGHTNESS;
    data.ledBudgetMa = DEFAULT_LED_BUDGET_MA;
    data.colorMode = COLOR_RANDOM;
    data.singleColor = 0x00FFFFFF; // white
    data.gradientVariant = GRAD_STATIC_INDEX;
    data.gradientSpeed = 40;
    data.animationEnabled = 1;
    data.animationId = 0;
    data.chimeEnabled = 0;
}

void Settings::resetDefaults()
{
    applyDefaults();
}

void Settings::begin()
{
    prefs.begin(NVS_NAMESPACE, /*readOnly=*/false);

    SettingsData loaded;
    size_t got = prefs.getBytes(NVS_BLOB_KEY, &loaded, sizeof(loaded));

    if (got == sizeof(loaded) && loaded.magic == SETTINGS_MAGIC &&
        loaded.version == SETTINGS_VERSION)
    {
        data = loaded;
        SERIAL_PRINTLN(F("[Settings] loaded from NVS"));
    }
    else
    {
        SERIAL_PRINTLN(F("[Settings] no/invalid NVS blob -> defaults"));
        applyDefaults();
        save();
    }
}

bool Settings::save()
{
    data.magic = SETTINGS_MAGIC;
    data.version = SETTINGS_VERSION;
    size_t written = prefs.putBytes(NVS_BLOB_KEY, &data, sizeof(data));
    bool ok = (written == sizeof(data));
    SERIAL_PRINTF("[Settings] save %s\n", ok ? "ok" : "FAILED");
    return ok;
}

bool Settings::hasWiFiCreds() const
{
    return data.ssid[0] != '\0';
}

void Settings::setWiFi(const char *ssid, const char *pass)
{
    copyStr(data.ssid, sizeof(data.ssid), ssid);
    copyStr(data.pass, sizeof(data.pass), pass);
}

void Settings::clearWiFi()
{
    data.ssid[0] = '\0';
    data.pass[0] = '\0';
}
