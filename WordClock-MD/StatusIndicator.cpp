// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Matt Disher
#include "StatusIndicator.h"
#include "SerialHelper.h"

// State colors (0x00RRGGBB).
#define COL_BOOT 0x202020
#define COL_PORTAL 0x0040FF
#define COL_CONNECTING 0xFF7000
#define COL_RUN_OK 0x00FF00
#define COL_WIFI_LOST 0xFF0000
#define COL_NTP_WAIT 0xFF00FF
#define COL_OTA 0xFFFFFF

#define GRID_ANIM_FRAME_MS 90UL      // random-letter refresh cadence
#define GRID_ANIM_LETTERS 16         // lit letters per full-grid frame
#define OVERLAY_PERIOD_MS 8000UL     // how often the warning flash appears
#define OVERLAY_ON_MS 900UL          // how long each flash lasts
#define OVERLAY_LETTERS 5            // letters lit during a warning flash
#define ONBOARD_MAX_BRIGHT 40        // the onboard LED is tiny but bright

StatusIndicator::StatusIndicator(ClockDisplayHAL *hal)
    : hal(hal),
      onboard(STATUS_LED_COUNT, STATUS_LED_PIN, NEO_GRB + NEO_KHZ800),
      current(STATUS_BOOT), lastGridMs(0), lastOverlayMs(0),
      lastOnboardMs(0), overlayVisible(false) {}

void StatusIndicator::setup()
{
    onboard.begin();
    onboard.setBrightness(255); // we scale colors ourselves
    onboard.clear();
    onboard.show();
}

void StatusIndicator::setState(StatusState s)
{
    if (s != current)
    {
        current = s;
        SERIAL_PRINTF("[Status] state -> %d\n", (int)s);
    }
}

uint32_t StatusIndicator::stateColor() const
{
    switch (current)
    {
    case STATUS_PORTAL:     return COL_PORTAL;
    case STATUS_CONNECTING: return COL_CONNECTING;
    case STATUS_RUN_OK:     return COL_RUN_OK;
    case STATUS_WIFI_LOST:  return COL_WIFI_LOST;
    case STATUS_NTP_WAIT:   return COL_NTP_WAIT;
    case STATUS_OTA:        return COL_OTA;
    case STATUS_BOOT:
    default:                return COL_BOOT;
    }
}

uint8_t StatusIndicator::pulse(uint32_t now, uint16_t periodMs) const
{
    uint32_t p = now % periodMs;
    uint32_t half = periodMs / 2;
    uint32_t v = (p < half) ? p : (periodMs - p); // triangle 0..half
    return (uint8_t)((v * 255) / half);
}

void StatusIndicator::writeOnboard(uint32_t color, uint8_t brightness)
{
    uint16_t r = (color >> 16) & 0xFF, g = (color >> 8) & 0xFF, b = color & 0xFF;
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;
    onboard.setPixelColor(0, onboard.Color(r, g, b));
    onboard.show();
}

void StatusIndicator::update(uint32_t now)
{
    // Throttle onboard-LED writes; pulses still look smooth at ~25 fps.
    if (now - lastOnboardMs < 40)
        return;
    lastOnboardMs = now;

    uint8_t bright = ONBOARD_MAX_BRIGHT;
    switch (current)
    {
    case STATUS_CONNECTING:
    case STATUS_NTP_WAIT:
        bright = (pulse(now, 1200) * ONBOARD_MAX_BRIGHT) / 255; // breathe
        break;
    case STATUS_OTA:
        bright = (pulse(now, 400) * ONBOARD_MAX_BRIGHT) / 255; // fast blink
        break;
    case STATUS_RUN_OK:
        bright = ONBOARD_MAX_BRIGHT / 3; // steady dim green
        break;
    default:
        break;
    }
    writeOnboard(stateColor(), bright);
}

bool StatusIndicator::ownsDisplay() const
{
    // These states take the whole grid. NTP_WAIT is included so we never paint
    // a bogus clock before the RTC has a valid time.
    return current == STATUS_BOOT || current == STATUS_PORTAL ||
           current == STATUS_CONNECTING || current == STATUS_OTA ||
           current == STATUS_NTP_WAIT;
}

void StatusIndicator::renderGrid(uint32_t now)
{
    if (now - lastGridMs < GRID_ANIM_FRAME_MS)
        return;
    lastGridMs = now;

    uint32_t base = stateColor();
    hal->clear();
    for (uint8_t i = 0; i < GRID_ANIM_LETTERS; i++)
    {
        uint16_t idx = random(NUM_LEDS);
        // slight per-letter brightness twinkle
        uint8_t b = 120 + random(136);
        uint16_t r = ((base >> 16) & 0xFF) * b / 255;
        uint16_t g = ((base >> 8) & 0xFF) * b / 255;
        uint16_t bl = (base & 0xFF) * b / 255;
        hal->setIndex(idx, ClockDisplayHAL::Color(r, g, bl));
    }
    hal->show();
}

bool StatusIndicator::hasOverlay() const
{
    // WIFI_LOST flashes over a still-valid (RTC-kept) clock.
    return current == STATUS_WIFI_LOST;
}

void StatusIndicator::applyOverlay(uint32_t now)
{
    // Flash a few random warning-colored letters over the clock periodically,
    // so time is only obscured for a moment.
    uint32_t phase = now % OVERLAY_PERIOD_MS;
    overlayVisible = (phase < OVERLAY_ON_MS);
    if (!overlayVisible)
        return;

    uint32_t base = stateColor();
    for (uint8_t i = 0; i < OVERLAY_LETTERS; i++)
        hal->setIndex(random(NUM_LEDS), base);
}
