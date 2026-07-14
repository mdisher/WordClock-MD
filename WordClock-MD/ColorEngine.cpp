// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#include "ColorEngine.h"
#include "Pins.h"
#include <Adafruit_NeoPixel.h>

// Palette for COLOR_RANDOM (inherited from the original clock's COLORS[]).
static const uint32_t PALETTE[] = {
    0xFF0000, // red
    0x00FF00, // green
    0x0000FF, // blue
    0xFFFF00, // yellow
    0xFF00FF, // magenta
    0x00FFFF, // cyan
    0xFFFFFF, // white
    0xA52A2A, // brown
};
static const uint16_t PALETTE_SIZE = sizeof(PALETTE) / sizeof(PALETTE[0]);

// Hue spread between successive word slots for per-word gradients.
#define HUE_STEP_PER_SLOT 7000

void ColorEngine::newMinute()
{
    // Mix the previous seed forward; deterministic, decorrelated per minute.
    minuteSeed = minuteSeed * 1664525u + 1013904223u;
}

bool ColorEngine::isSpatial() const
{
    return settings->data.colorMode == COLOR_GRADIENT &&
           settings->data.gradientVariant == GRAD_SPATIAL;
}

uint16_t ColorEngine::timePhase(uint32_t nowMs) const
{
    // gradientSpeed 0..255 scales how fast the hue phase advances.
    uint32_t speed = settings->data.gradientSpeed + 1u;
    return (uint16_t)((nowMs * speed) / 16u);
}

uint32_t ColorEngine::colorForSlot(uint16_t slot, uint32_t nowMs) const
{
    switch (settings->data.colorMode)
    {
    case COLOR_SINGLE:
        return settings->data.singleColor;

    case COLOR_GRADIENT:
    {
        uint16_t hue = (uint16_t)(slot * HUE_STEP_PER_SLOT);
        if (settings->data.gradientVariant == GRAD_CYCLE_INDEX)
            hue += timePhase(nowMs);
        // GRAD_SPATIAL is handled per-pixel by colorForXY(); if we somehow get
        // here in that mode, fall back to a per-slot hue.
        return Adafruit_NeoPixel::ColorHSV(hue, 255, 255);
    }

    case COLOR_RANDOM:
    default:
    {
        // Deterministic per-(slot, minute) palette pick. Hashing instead of
        // random() means the same word keeps the same color for the whole
        // minute (no per-frame flicker), yet the assignment looks shuffled and
        // changes each minute as newMinute() advances minuteSeed.
        //   2654435761u = Knuth's 32-bit multiplicative-hash constant (2^32/phi)
        //   40503u      = an odd mixer so the seed's bits spread before XOR
        //   h ^= h >> 13 = an xorshift finalizer to decorrelate low bits
        uint32_t h = (uint32_t)(slot + 1) * 2654435761u ^ (minuteSeed * 40503u);
        h ^= h >> 13;
        return PALETTE[h % PALETTE_SIZE];
    }
    }
}

uint32_t ColorEngine::colorForXY(uint8_t x, uint8_t y, uint32_t nowMs) const
{
    // Diagonal rainbow across the face that drifts over time. This is the
    // "whole-display shifting gradient" the user wants to audition.
    uint32_t hx = (uint32_t)x * (65535u / GRID_WIDTH);
    uint32_t hy = (uint32_t)y * (65535u / GRID_HEIGHT) / 2u;
    uint16_t hue = (uint16_t)(hx + hy) + timePhase(nowMs);
    return Adafruit_NeoPixel::ColorHSV(hue, 255, 255);
}
