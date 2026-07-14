#ifndef PINS_H
#define PINS_H

// =====================================================================
//  Pin + grid geometry for the ESP32-S3 SuperMini (HW-747 v0.0.2)
// =====================================================================
//
//  LED strip data leaves GPIO1 through a 330 ohm series resistor straight
//  to the strip's DIN -- NO level shifter. The ESP32-S3's 3.3V data is
//  reshaped by the first WS2812 and drives the chain reliably over the
//  short lead used here. (A 74AHCT125 push-pull buffer is the belt-and-
//  suspenders option if a long data run ever misbehaves, but the shipped
//  build runs direct.) GPIO1 is a plain output on the S3 -- not a strapping
//  pin -- and leaves UART0 (TX/RX) free for serial debugging.
//
//  Build note: keep the board's PCB antenna (far end from USB-C) in clear
//  air -- metal/wiring within its keep-out zone detunes it and kills Wi-Fi.
//
//  The physical panel is 132 WS2812B LEDs: 11 rows of 12, wired in a
//  serpentine (boustrophedon) layout. This matches johniak's word-clock
//  frame, so WORDS_TO_LEDS[] in ClockDisplayHAL is reused verbatim.

// ---- LED strip (word grid) ----
#define DATA_PIN 1
#define GRID_WIDTH 12
#define GRID_HEIGHT 11
#define NUM_LEDS (GRID_WIDTH * GRID_HEIGHT) // 132

// ---- Onboard status LED (single WS2812 on the SuperMini) ----
#define STATUS_LED_PIN 48
#define STATUS_LED_COUNT 1

#endif // PINS_H
