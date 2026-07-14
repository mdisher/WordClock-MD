#ifndef COLOR_ENGINE_H
#define COLOR_ENGINE_H

#include <Arduino.h>
#include "Settings.h"

// =====================================================================
//  ColorEngine
//  Turns the active color-mode setting into per-word (or per-pixel)
//  colors. All colors are returned at full value (0x00RRGGBB); global
//  brightness + the current clamp are applied later by ClockDisplayHAL.
//
//  Modes:
//    COLOR_RANDOM   - each word slot a palette color, stable within a
//                     minute (re-rolled by newMinute()), no per-second flicker.
//    COLOR_SINGLE   - every word the one configured color.
//    COLOR_GRADIENT - GRAD_STATIC_INDEX / GRAD_CYCLE_INDEX (per-word hue,
//                     optionally advancing) or GRAD_SPATIAL (a whole-face
//                     rainbow that drifts, applied per-pixel).
// =====================================================================
class ColorEngine
{
public:
    explicit ColorEngine(Settings *settings) : settings(settings), minuteSeed(1) {}

    // Called when the displayed minute changes; re-rolls random assignments
    // and can advance any per-minute state.
    void newMinute();

    // Is the current mode a per-pixel spatial gradient? (WordClock uses this
    // to decide between whole-word coloring and per-LED coloring.)
    bool isSpatial() const;

    // Color for the Nth lit word this frame (slot = 0,1,2,...).
    uint32_t colorForSlot(uint16_t slot, uint32_t nowMs) const;

    // Color for a grid position (used only when isSpatial()).
    uint32_t colorForXY(uint8_t x, uint8_t y, uint32_t nowMs) const;

private:
    Settings *settings;
    uint32_t minuteSeed;

    uint16_t timePhase(uint32_t nowMs) const; // 0..65535, advances with gradientSpeed
};

#endif // COLOR_ENGINE_H
