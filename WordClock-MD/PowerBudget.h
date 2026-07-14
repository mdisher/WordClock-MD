// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#ifndef POWER_BUDGET_H
#define POWER_BUDGET_H

#include <Arduino.h>
#include "Config.h"

// =====================================================================
//  PowerBudget
//  Estimates WS2812B strip current from a logical (full-intensity)
//  framebuffer and computes the highest brightness that stays within a
//  configured milliamp budget. This is what makes the clock safe on a
//  fixed supply: no matter what the words or an animation draw, show()
//  never lets the strip exceed the budget.
//
//  Current model (per LED): idle draw (always present) plus each color
//  channel contributing up to LED_MA_PER_CHANNEL_FULL at full value,
//  scaled linearly by the global brightness factor.
// =====================================================================
class PowerBudget
{
public:
    // Total estimated current (mA) for `buffer` displayed at `brightness` (0..255).
    static uint32_t estimateCurrentMa(const uint32_t *buffer, uint16_t count,
                                      uint8_t brightness);

    // Highest brightness (<= desired) whose estimate stays within budgetMa.
    static uint8_t clampBrightness(const uint32_t *buffer, uint16_t count,
                                   uint8_t desiredBrightness, uint16_t budgetMa);

private:
    // Sum of all channel intensities (each 0..255) across the buffer.
    static uint32_t channelSum(const uint32_t *buffer, uint16_t count);
};

#endif // POWER_BUDGET_H
