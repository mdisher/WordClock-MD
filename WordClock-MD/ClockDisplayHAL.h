#ifndef CLOCKDISPLAYHAL_H
#define CLOCKDISPLAYHAL_H

#include <Adafruit_NeoPixel.h>
#include "Pins.h"

// =====================================================================
//  ClockDisplayHAL
//  Owns the 132-LED word grid. Drawing operations write into a *logical*
//  full-intensity framebuffer (uint32_t 0x00RRGGBB per LED). show() then
//  applies our own global brightness scaling AND the PowerBudget current
//  clamp before pushing to the strip -- so Adafruit's lossy setBrightness
//  is left disabled (255) and the current ceiling is always enforced.
// =====================================================================
class ClockDisplayHAL
{
public:
    static const uint16_t WIDTH = GRID_WIDTH;
    static const uint16_t HEIGHT = GRID_HEIGHT;
    static const uint16_t NUM_PIXELS = NUM_LEDS;

    ClockDisplayHAL(uint8_t pin, uint8_t brightness, uint16_t ledBudgetMa);

    Adafruit_NeoPixel pixels;

    void setup();

    // ---- configuration (live-updatable from the dashboard) ----
    void setBrightness(uint8_t b) { userBrightness = b; }
    uint8_t getBrightness() const { return userBrightness; }
    void setBudgetMa(uint16_t ma) { ledBudgetMa = ma; }
    uint16_t getBudgetMa() const { return ledBudgetMa; }

    // ---- drawing (into the logical buffer; call show() to push) ----
    void clear();                                     // buffer -> all off
    void setIndex(uint16_t i, uint32_t color);        // raw strip index
    void setPixel(uint8_t x, uint8_t y, uint32_t color); // cartesian -> serpentine
    void fill(uint32_t color);
    void displayWord(const String &word, uint32_t color);

    // ---- output ----
    void show();      // clamp + scale + push to strip
    void showOff();   // force everything off immediately

    // ---- telemetry (for the dashboard/diagnostics) ----
    uint32_t estimateCurrentMa() const;      // at the effective brightness last used
    uint8_t lastEffectiveBrightness() const { return effBrightness; }

    // ---- helpers ----
    static uint32_t Color(uint8_t r, uint8_t g, uint8_t b)
    {
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
    uint16_t cartesianToStripIndex(uint8_t x, uint8_t y) const;
    void indexToXY(uint16_t i, uint8_t &x, uint8_t &y) const; // inverse of the above

    // Look up a word's inclusive strip-index range. Returns false if unknown.
    bool wordRange(const String &word, uint8_t &start, uint8_t &end) const;

    // Number of words known to the panel (for diagnostic/test mode).
    static uint16_t wordCount();
    static const char *wordName(uint16_t i);

private:
    uint8_t userBrightness;
    uint16_t ledBudgetMa;
    uint8_t effBrightness; // brightness actually used at last show()

    uint32_t buffer[NUM_LEDS]; // logical, full-intensity colors

    static const struct WordMapping
    {
        const char *word;
        uint8_t start;
        uint8_t end;
    } WORDS_TO_LEDS[];
    static const uint16_t WORD_MAP_COUNT;
};

#endif // CLOCKDISPLAYHAL_H
