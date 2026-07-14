// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#include "PowerBudget.h"

uint32_t PowerBudget::channelSum(const uint32_t *buffer, uint16_t count)
{
    uint32_t sum = 0;
    for (uint16_t i = 0; i < count; i++)
    {
        uint32_t c = buffer[i];
        sum += ((c >> 16) & 0xFF) + ((c >> 8) & 0xFF) + (c & 0xFF);
    }
    return sum;
}

uint32_t PowerBudget::estimateCurrentMa(const uint32_t *buffer, uint16_t count,
                                        uint8_t brightness)
{
    float bf = brightness / 255.0f;
    float colorMa = (channelSum(buffer, count) / 255.0f) *
                    LED_MA_PER_CHANNEL_FULL * bf;
    float idleMa = count * LED_MA_IDLE_PER_LED;
    return (uint32_t)(idleMa + colorMa + 0.5f);
}

uint8_t PowerBudget::clampBrightness(const uint32_t *buffer, uint16_t count,
                                     uint8_t desiredBrightness, uint16_t budgetMa)
{
    // Fast path: desired already fits.
    if (estimateCurrentMa(buffer, count, desiredBrightness) <= budgetMa)
        return desiredBrightness;

    // Only the color-dependent term scales with brightness; idle is fixed.
    float idleMa = count * LED_MA_IDLE_PER_LED;
    float colorMaFull = (channelSum(buffer, count) / 255.0f) *
                        LED_MA_PER_CHANNEL_FULL; // at brightness factor 1.0

    if (colorMaFull <= 0.0f)
        return desiredBrightness; // nothing lit -> nothing to clamp

    float budgetForColor = (float)budgetMa - idleMa;
    if (budgetForColor <= 0.0f)
        return 0; // idle alone exceeds budget (shouldn't happen with sane budgets)

    float bfMax = budgetForColor / colorMaFull;
    if (bfMax > 1.0f)
        bfMax = 1.0f;

    uint8_t bMax = (uint8_t)(bfMax * 255.0f);
    return (bMax < desiredBrightness) ? bMax : desiredBrightness;
}
