// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#ifndef CONFIG_H
#define CONFIG_H

#include "Pins.h"

// =====================================================================
//  Build-time constants (non-secret). WiFi credentials and user
//  preferences live in NVS via Settings, NOT here.
// =====================================================================

// ---- Serial debug ----
#define USE_SERIAL 1
#define SERIAL_BAUD 115200

// ---- Captive-portal access point (shown when unprovisioned) ----
#define AP_SSID "WordClock-Setup"
#define AP_PASSWORD "" // open network; portal has no secrets to protect
#define DNS_PORT 53
#define WEB_PORT 80

// ---- mDNS / default hostname ----
#define DEFAULT_HOSTNAME "wordclock"

// ---- Time / NTP ----
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov"
// Default POSIX TZ string. US Eastern with automatic DST.
// (Editable in the dashboard; see WebPortal timezone list.)
#define DEFAULT_TZ "EST5EDT,M3.2.0,M11.1.0"
#define NTP_RESYNC_INTERVAL_MS (24UL * 60UL * 60UL * 1000UL) // daily

// ---- WiFi connection behaviour ----
#define WIFI_CONNECT_TIMEOUT_MS 15000UL // STA attempt before falling back to portal
#define WIFI_RETRY_INTERVAL_MS 30000UL  // background reconnect cadence once in RUN

// ---- Power / display defaults (overridable in the dashboard) ----
// Sized for the external 5V / 3A supply: 2400 mA for LEDs leaves ~600 mA
// of headroom for the ESP32-S3 (WiFi bursts) plus margin.
#define DEFAULT_BRIGHTNESS 64      // 25% of 255
#define DEFAULT_LED_BUDGET_MA 2400 // firmware current clamp ceiling
#define MAX_LED_BUDGET_MA 4000     // UI upper bound (bigger PSU territory)

// WS2812B current model used by PowerBudget: ~20 mA per fully-lit channel,
// ~1 mA quiescent per LED. Full white ~= 61 mA.
#define LED_MA_PER_CHANNEL_FULL 20.0f
#define LED_MA_IDLE_PER_LED 1.0f

// ---- Animation ----
#define ANIMATION_DURATION_MS 4000UL
#define ANIMATION_FRAME_MS 33UL // ~30 fps

#endif // CONFIG_H
