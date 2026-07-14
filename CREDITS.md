# Credits & Attribution

## Original inspiration

WordClock-MD began as a modernization of
**[johniak/word-clock](https://github.com/johniak/word-clock)**. Credit where it's due — the
following originate from that project:

- the physical word-grid frame and letter template,
- the 11 × 12 serpentine (boustrophedon) LED layout, and
- the English word mapping (`WORDS_TO_LEDS[]`) and the time → words phrasing.

A copy of the original project is included **unmodified** under [`wordclock/`](wordclock/) as a
reference.

> **Licensing note.** The original repository specifies **no license**, so by default its author
> retains all rights to it. The copy under `wordclock/` is included **for reference only**. The
> GPL-3.0 license in this repository's [`LICENSE`](LICENSE) applies to the **WordClock-MD firmware**
> (the from-scratch rewrite under `WordClock-MD/`) — it does **not** apply to, and makes no claim
> over, the contents of `wordclock/`, which remain their author's.

## This project

The **WordClock-MD firmware** (everything under [`WordClock-MD/`](WordClock-MD/)) is an original,
from-scratch, fully non-blocking rewrite for the Arduino IDE / ESP32-S3 SuperMini: captive-portal
Wi-Fi setup, a self-contained mobile dashboard, OTA updates, POSIX-TZ NTP with automatic DST, three
color modes, non-blocking on-the-hour animations, a random-letter status indicator, and a per-frame
current clamp that keeps the WS2812B panel safe on a fixed supply.

Copyright © 2026 Matt Disher. Licensed under **GPL-3.0-or-later** — see [`LICENSE`](LICENSE).

## Third-party references

- [`ESP32-Mini Docs/`](ESP32-Mini%20Docs/) — ESP32-S3 SuperMini getting-started material and board
  images, © their respective owners (Espressif Systems / the board vendor). Included for
  convenience; not covered by this repository's license.

## Libraries

Built on these open-source libraries, each under its own license:

- **Adafruit NeoPixel** — WS2812B driver
- **ESP Async WebServer** + **Async TCP** (ESP32Async fork) — the async HTTP server
- **ArduinoJson** (v7) — JSON for the API
- **Arduino-ESP32 core** (Espressif) — WiFi, Preferences, ESPmDNS, DNSServer, Update, `time.h`
