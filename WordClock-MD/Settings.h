// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include "Config.h"

// ---- Color modes (see ColorEngine) ----
enum ColorMode : uint8_t
{
    COLOR_RANDOM = 0,   // each lit word a random palette color, re-rolled on the minute
    COLOR_SINGLE = 1,   // all lit words share one configured color
    COLOR_GRADIENT = 2, // per-word / spatial hue (see GradientVariant)
};

// ---- Gradient sub-variants (auditioned on the real panel, then tuned) ----
enum GradientVariant : uint8_t
{
    GRAD_STATIC_INDEX = 0, // hue derived from word index, fixed
    GRAD_CYCLE_INDEX = 1,  // hue by word index, slowly advancing over time
    GRAD_SPATIAL = 2,      // hue by grid position (a soft rainbow across the face)
};

// POD blob persisted to NVS. Kept trivially-copyable so Preferences can
// putBytes/getBytes it in one shot with a version guard.
struct SettingsData
{
    uint16_t magic;   // validity marker
    uint8_t version;  // schema version, bump on layout change
    char ssid[33];    // 32 chars + NUL
    char pass[65];    // 64 chars + NUL
    char tz[48];      // POSIX TZ string
    char hostname[24];
    uint8_t brightness;   // 0..255 (user ceiling; clamp may lower it live)
    uint16_t ledBudgetMa; // firmware current clamp
    uint8_t colorMode;    // ColorMode
    uint32_t singleColor; // 0x00RRGGBB, used by COLOR_SINGLE
    uint8_t gradientVariant;
    uint8_t gradientSpeed; // 0..255, phase advance rate for cycling gradients
    uint8_t animationEnabled;
    uint8_t animationId;
    uint8_t chimeEnabled; // reserved for a future hourly chime
};

class Settings
{
public:
    SettingsData data;

    void begin();       // load from NVS, or seed + save defaults
    bool save();        // persist current data to NVS
    void resetDefaults(); // in-memory reset (does not auto-save)

    bool hasWiFiCreds() const;
    void setWiFi(const char *ssid, const char *pass);
    void clearWiFi();

private:
    void applyDefaults();
};

#endif // SETTINGS_H
