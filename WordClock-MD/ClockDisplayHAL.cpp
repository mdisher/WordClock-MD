// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#include "ClockDisplayHAL.h"
#include "PowerBudget.h"

// Word -> raw strip-index ranges (inclusive). Inherited verbatim from
// johniak's word-clock; matches the printed letter template.
ClockDisplayHAL::WordMapping const ClockDisplayHAL::WORDS_TO_LEDS[] = {
    {"HOUR_1", 20, 22},
    {"HOUR_2", 45, 47},
    {"HOUR_3", 15, 19},
    {"HOUR_4", 67, 70},
    {"HOUR_5", 40, 43},
    {"HOUR_6", 12, 14},
    {"HOUR_7", 55, 59},
    {"HOUR_8", 31, 35},
    {"HOUR_9", 36, 39},
    {"HOUR_10", 9, 11},
    {"HOUR_11", 24, 29},
    {"HOUR_12", 48, 53},
    {"OCLOCK", 0, 5},
    {"PAST", 60, 63},
    {"TO", 63, 64},
    {"MINUTES", 77, 83},
    {"THIRTY", 84, 89},
    {"TWENTY", 102, 107},
    {"TWENTYFIVE", 98, 107},
    {"FIVE", 98, 101},
    {"TEN", 91, 93},
    {"FIFTEEN", 110, 116},
    {"IS", 127, 128},
    {"IT", 130, 131}};

const uint16_t ClockDisplayHAL::WORD_MAP_COUNT =
    sizeof(ClockDisplayHAL::WORDS_TO_LEDS) / sizeof(ClockDisplayHAL::WORDS_TO_LEDS[0]);

ClockDisplayHAL::ClockDisplayHAL(uint8_t pin, uint8_t brightness, uint16_t ledBudgetMa)
    : pixels(NUM_LEDS, pin, NEO_GRB + NEO_KHZ800),
      userBrightness(brightness),
      ledBudgetMa(ledBudgetMa),
      effBrightness(brightness)
{
    memset(buffer, 0, sizeof(buffer));
}

void ClockDisplayHAL::setup()
{
    pixels.begin();
    // We do our own brightness scaling + current clamp in show(); keep the
    // library's (lossy) global brightness fully open.
    pixels.setBrightness(255);
    pixels.clear();
    pixels.show();
}

// -------- scaling helper --------
static inline uint32_t scaleColor(uint32_t c, uint8_t b)
{
    if (b >= 255)
        return c;
    uint16_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, bl = c & 0xFF;
    r = (r * b) / 255;
    g = (g * b) / 255;
    bl = (bl * b) / 255;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | bl;
}

// -------- drawing --------
void ClockDisplayHAL::clear()
{
    memset(buffer, 0, sizeof(buffer));
}

void ClockDisplayHAL::setIndex(uint16_t i, uint32_t color)
{
    if (i < NUM_LEDS)
        buffer[i] = color;
}

void ClockDisplayHAL::fill(uint32_t color)
{
    for (uint16_t i = 0; i < NUM_LEDS; i++)
        buffer[i] = color;
}

void ClockDisplayHAL::displayWord(const String &word, uint32_t color)
{
    for (uint16_t m = 0; m < WORD_MAP_COUNT; m++)
    {
        if (word.equals(WORDS_TO_LEDS[m].word))
        {
            for (uint8_t i = WORDS_TO_LEDS[m].start; i <= WORDS_TO_LEDS[m].end; ++i)
                setIndex(i, color);
            return;
        }
    }
}

uint16_t ClockDisplayHAL::cartesianToStripIndex(uint8_t x, uint8_t y) const
{
    // Serpentine (boustrophedon) mapping, preserved verbatim from the original
    // build and validated on the real panel via Diagnostics -> "Cycle all".
    // The strip is folded into 11 rows of 12; the data chain starts at the far
    // corner, so HIGHER grid rows (larger y) map to LOWER strip indices, and
    // the scan direction alternates every row:
    //   even y: index runs right-to-left (index decreases as x increases)
    //   odd  y: index runs left-to-right (index increases as x increases)
    // e.g. y=0 occupies strip indices 120..131; y=1 occupies 108..119.
    uint16_t row_index;
    uint16_t index;

    if (y % 2 == 0)
    {
        row_index = NUM_LEDS - (y * WIDTH);
        index = row_index - (x + 1);
    }
    else
    {
        row_index = NUM_LEDS - ((y + 1) * WIDTH);
        index = row_index + x;
    }

    if (index >= NUM_LEDS)
        return 0;
    return index;
}

void ClockDisplayHAL::indexToXY(uint16_t i, uint8_t &x, uint8_t &y) const
{
    // Inverse of cartesianToStripIndex(). Rows are 12-LED bands stacked from
    // the strip's tail; even/odd rows run in opposite directions.
    if (i >= NUM_LEDS)
    {
        x = 0;
        y = 0;
        return;
    }
    y = (uint8_t)(((NUM_LEDS - 1) - i) / WIDTH);
    uint16_t bandTop = (NUM_LEDS - 1) - (uint16_t)WIDTH * y; // highest index in this row
    uint16_t bandBottom = bandTop - (WIDTH - 1);
    x = (y % 2 == 0) ? (uint8_t)(bandTop - i) : (uint8_t)(i - bandBottom);
}

bool ClockDisplayHAL::wordRange(const String &word, uint8_t &start, uint8_t &end) const
{
    for (uint16_t m = 0; m < WORD_MAP_COUNT; m++)
    {
        if (word.equals(WORDS_TO_LEDS[m].word))
        {
            start = WORDS_TO_LEDS[m].start;
            end = WORDS_TO_LEDS[m].end;
            return true;
        }
    }
    return false;
}

void ClockDisplayHAL::setPixel(uint8_t x, uint8_t y, uint32_t color)
{
    setIndex(cartesianToStripIndex(x, y), color);
}

// -------- output --------
void ClockDisplayHAL::show()
{
    // Enforce the current budget: never draw more than ledBudgetMa.
    effBrightness = PowerBudget::clampBrightness(buffer, NUM_LEDS,
                                                 userBrightness, ledBudgetMa);
    for (uint16_t i = 0; i < NUM_LEDS; i++)
        pixels.setPixelColor(i, scaleColor(buffer[i], effBrightness));
    pixels.show();
}

void ClockDisplayHAL::showOff()
{
    clear();
    pixels.clear();
    pixels.show();
    effBrightness = 0;
}

// -------- telemetry --------
uint32_t ClockDisplayHAL::estimateCurrentMa() const
{
    return PowerBudget::estimateCurrentMa(buffer, NUM_LEDS, effBrightness);
}

// -------- word introspection (diagnostic/test mode) --------
uint16_t ClockDisplayHAL::wordCount()
{
    return WORD_MAP_COUNT;
}

const char *ClockDisplayHAL::wordName(uint16_t i)
{
    if (i >= WORD_MAP_COUNT)
        return "";
    return WORDS_TO_LEDS[i].word;
}
