#ifndef ANIMATOR_H
#define ANIMATOR_H

#include <Arduino.h>
#include "ClockDisplayHAL.h"

// =====================================================================
//  Animator
//  Non-blocking, frame-based on-the-hour effects. Replaces the original
//  blocking GIF player: start() arms it, update() advances one frame per
//  ANIMATION_FRAME_MS, and it self-terminates after ANIMATION_DURATION_MS.
//  While active, WordClock yields the display to it. Every frame is pushed
//  through ClockDisplayHAL::show(), so the PowerBudget clamp still governs
//  current even for full-face effects.
//
//  A future hourly chime can be triggered alongside start() (see the hook).
// =====================================================================
class Animator
{
public:
    explicit Animator(ClockDisplayHAL *hal) : hal(hal), active(false),
                                              effect(0), startMs(0),
                                              lastFrameMs(0), frame(0) {}

    // animId: 0..effectCount()-1 selects an effect; ANIM_RANDOM picks one.
    void start(uint8_t animId, uint32_t now);
    void update(uint32_t now);
    void stop();
    bool isActive() const { return active; }

    static uint8_t effectCount();
    static const char *effectName(uint8_t i);

    static const uint8_t ANIM_RANDOM = 255;

private:
    ClockDisplayHAL *hal;
    bool active;
    uint8_t effect;
    uint32_t startMs;
    uint32_t lastFrameMs;
    uint32_t frame;

    void renderFrame(uint32_t now);
    void drawSparkle(uint32_t now);
    void drawSweep(uint32_t now);
    void drawRainbow(uint32_t now);
};

#endif // ANIMATOR_H
