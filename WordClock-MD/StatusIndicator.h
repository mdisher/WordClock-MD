// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#ifndef STATUS_INDICATOR_H
#define STATUS_INDICATOR_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "ClockDisplayHAL.h"

// =====================================================================
//  StatusIndicator
//  Signals system state two ways:
//    - the onboard WS2812 on GPIO48 (always shows a color/pulse), and
//    - random letters on the main grid, in the state color.
//
//  Ownership model (the .ino orchestrates):
//    ownsDisplay() states (BOOT/PORTAL/CONNECTING/OTA) -> renderGrid()
//      draws a full random-letter animation (the clock isn't showing then).
//    overlay states (WIFI_LOST/NTP_WAIT) during RUN -> applyOverlay()
//      briefly flashes a few random letters over the clock, then yields.
//    RUN_OK -> onboard LED only; the clock owns the grid.
// =====================================================================
enum StatusState : uint8_t
{
    STATUS_BOOT = 0,
    STATUS_PORTAL,
    STATUS_CONNECTING,
    STATUS_RUN_OK,
    STATUS_WIFI_LOST,
    STATUS_NTP_WAIT,
    STATUS_OTA,
};

class StatusIndicator
{
public:
    explicit StatusIndicator(ClockDisplayHAL *hal);

    void setup();
    void setState(StatusState s);
    StatusState state() const { return current; }

    void update(uint32_t now); // drives the onboard LED

    bool ownsDisplay() const;      // status animation should fill the grid
    void renderGrid(uint32_t now); // full-grid random-letter animation + show()

    bool hasOverlay() const;             // brief warning flash during RUN
    void applyOverlay(uint32_t now);     // tweak a few grid pixels (caller show()s)

private:
    ClockDisplayHAL *hal;
    Adafruit_NeoPixel onboard;
    StatusState current;

    uint32_t lastGridMs;
    uint32_t lastOverlayMs;
    uint32_t lastOnboardMs;
    bool overlayVisible;

    uint32_t stateColor() const;                 // 0x00RRGGBB for the current state
    uint8_t pulse(uint32_t now, uint16_t periodMs) const; // 0..255 triangle
    void writeOnboard(uint32_t color, uint8_t brightness);
};

#endif // STATUS_INDICATOR_H
