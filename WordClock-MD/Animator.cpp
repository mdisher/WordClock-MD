// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#include "Animator.h"
#include "SerialHelper.h"
#include <Adafruit_NeoPixel.h>

static const char *const EFFECT_NAMES[] = {"Sparkle", "Sweep", "Rainbow"};
static const uint8_t EFFECT_COUNT = sizeof(EFFECT_NAMES) / sizeof(EFFECT_NAMES[0]);

uint8_t Animator::effectCount() { return EFFECT_COUNT; }

const char *Animator::effectName(uint8_t i)
{
    return (i < EFFECT_COUNT) ? EFFECT_NAMES[i] : "";
}

void Animator::start(uint8_t animId, uint32_t now)
{
    effect = (animId == ANIM_RANDOM) ? (uint8_t)random(EFFECT_COUNT)
                                     : (uint8_t)(animId % EFFECT_COUNT);
    active = true;
    startMs = now;
    lastFrameMs = 0;
    frame = 0;
    // Hook: trigger an hourly chime here in a future revision.
    SERIAL_PRINTF("[Anim] start effect \"%s\"\n", effectName(effect));
}

void Animator::stop()
{
    active = false;
    hal->clear();
    hal->show();
}

void Animator::update(uint32_t now)
{
    if (!active)
        return;

    if (now - startMs >= ANIMATION_DURATION_MS)
    {
        active = false; // WordClock will repaint the clock face next tick
        SERIAL_PRINTLN(F("[Anim] done"));
        return;
    }

    if (now - lastFrameMs >= ANIMATION_FRAME_MS)
    {
        lastFrameMs = now;
        renderFrame(now);
        frame++;
    }
}

void Animator::renderFrame(uint32_t now)
{
    hal->clear();
    switch (effect)
    {
    case 1:
        drawSweep(now);
        break;
    case 2:
        drawRainbow(now);
        break;
    case 0:
    default:
        drawSparkle(now);
        break;
    }
    hal->show();
}

// ---- Effect 0: Sparkle -- scattered random-hue points ----
void Animator::drawSparkle(uint32_t now)
{
    const uint8_t kSparks = 24; // modest count; clamp handles the rest
    for (uint8_t i = 0; i < kSparks; i++)
    {
        uint16_t idx = random(NUM_LEDS);
        uint16_t hue = random(65536);
        hal->setIndex(idx, Adafruit_NeoPixel::ColorHSV(hue, 255, 255));
    }
}

// ---- Effect 1: Sweep -- a hue bar wipes across the columns ----
void Animator::drawSweep(uint32_t now)
{
    uint8_t col = frame % ClockDisplayHAL::WIDTH;
    uint16_t hue = (uint16_t)(frame * 2000);
    for (uint8_t y = 0; y < ClockDisplayHAL::HEIGHT; y++)
    {
        hal->setPixel(col, y, Adafruit_NeoPixel::ColorHSV(hue, 255, 255));
        // trailing dimmer column for a little motion blur
        if (col > 0)
            hal->setPixel(col - 1, y,
                          Adafruit_NeoPixel::ColorHSV(hue - 4000, 255, 100));
    }
}

// ---- Effect 2: Rainbow -- full-face diagonal rainbow that rotates ----
void Animator::drawRainbow(uint32_t now)
{
    uint16_t phase = (uint16_t)(frame * 1500);
    for (uint8_t y = 0; y < ClockDisplayHAL::HEIGHT; y++)
    {
        for (uint8_t x = 0; x < ClockDisplayHAL::WIDTH; x++)
        {
            uint16_t hue = (uint16_t)((x + y) * (65535 / (ClockDisplayHAL::WIDTH +
                                                          ClockDisplayHAL::HEIGHT))) +
                           phase;
            hal->setPixel(x, y, Adafruit_NeoPixel::ColorHSV(hue, 255, 255));
        }
    }
}
