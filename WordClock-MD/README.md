# WordClock-MD

A modernized ESP32-S3 word clock, rebuilt from [johniak/word-clock](https://github.com/johniak/word-clock)
for the **Arduino IDE** and the **ESP32-S3 SuperMini (HW-747 v0.0.2)**.

Everything is **non-blocking** (millis-driven — no `delay()`, no busy-loops):

- **Captive-portal Wi-Fi setup** with a self-contained, mobile-first web UI
- **Settings dashboard** (brightness, color, timezone, animation) + **OTA** firmware updates
- **NTP time** via a POSIX **TZ string** → automatic DST, with a **daily resync**
- **Three color modes**: random-per-word, single color, and gradient (per-word static,
  per-word cycle, or whole-face drifting rainbow)
- **On-the-hour animations** (Sparkle / Sweep / Rainbow), non-blocking
- **Random-letter status indicator** for Wi-Fi / NTP / OTA state, mirrored on the onboard LED
- **Firmware current clamp**: the strip never exceeds a configurable mA budget, so it's safe on
  the external **5 V / 3 A** supply. Worst-case clock display is 31 LEDs.

> See the project [`README.md`](../README.md) for the full "how it works" walkthrough, and
> [`HARDWARE.md`](../HARDWARE.md) for the BOM + wiring.

---

## Hardware

- **Board:** ESP32-S3 SuperMini (HW-747 v0.0.2)
- **Strip:** 132 × WS2812B (74 LED/m, cut into 11 rows of 12, serpentine)
- **Data:** GPIO1 → 330 Ω → strip DIN (**direct 3.3 V — no level shifter**)
- **Onboard status LED:** WS2812 on GPIO48 (no wiring)
- **Power:** external 5 V / 3 A → distribution → strip (heavy) + board 5V pin (light).
  ≈940 µF (2×470 µF) bulk cap at the strip, common ground everywhere, inject 5 V/GND at both ends.
  See [`../HARDWARE.md`](../HARDWARE.md) and [`../WordClock-wiring.svg`](../WordClock-wiring.svg).

> Do **not** power the board from USB-C and the 5 V supply's board-tap at the same time. After the
> first flash, update over the air (OTA) and leave USB unplugged in normal use.
>
> Mount the board with its **antenna end (opposite USB-C) in clear air** — nearby metal/wiring
> detunes it and kills Wi-Fi.

Pins/geometry live in `Pins.h`; other build constants (defaults, AP name, NTP, timeouts) in `Config.h`.

---

## Arduino IDE setup

1. **Boards Manager** → install **esp32 by Espressif Systems**.
2. Select board **"ESP32S3 Dev Module"** with:
   - **USB CDC On Boot:** Enabled (so Serial works over USB-C)
   - **Partition Scheme:** an **OTA-capable** scheme, e.g. *"Minimal SPIFFS (1.9MB APP with OTA)"*
     or *"8M with spiffs (3MB APP/1.5MB FS)"* (match your flash size; SuperMini is usually 8 MB)
   - **PSRAM:** Disabled
   - Upload speed 921600
3. **Library Manager** → install:
   - **Adafruit NeoPixel**
   - **ESP Async WebServer** (ESP32Async fork) — provides `ESPAsyncWebServer.h` and `AsyncJson.h`
   - **Async TCP** (ESP32Async fork) — dependency of the above
   - **ArduinoJson** (v7)

   *(DNSServer, Preferences, ESPmDNS, WiFi, Update, and `time.h` ship with the ESP32 core.)*
4. Open `WordClock-MD.ino` (keep all files together in the `WordClock-MD/` folder) and upload.

---

## First run

1. On first boot (no saved Wi-Fi), the clock raises an open access point **`WordClock-Setup`** and
   the grid shows scattered **blue** letters (onboard LED blue).
2. Join it with a phone; the captive portal opens automatically (or browse to `http://192.168.4.1`).
3. **Wi-Fi** tab → *Scan* → pick your network (2.4 GHz) → password → **Connect & save**. The clock
   reboots and joins your network.
4. Once connected it syncs NTP (grid shows **magenta** letters until the first sync), then displays
   the time. Reach the dashboard at **`http://wordclock.local`** (or the IP shown on the chip).

## Dashboard

- **Clock:** brightness, current budget (mA), color mode + options, on-the-hour animation (with
  *Preview*), and timezone. Sliders update the clock live; **Save** persists to NVS.
- **Diagnostics:** light each word (or cycle/all) to verify the letter map against your printed
  template.
- **Wi-Fi:** re-provision the network.
- **System:** hostname, **OTA** firmware upload (`.bin`), and factory reset.

---

## Status colors

| State | Grid (random letters) | Onboard GPIO48 |
|---|---|---|
| Setup / captive portal | blue | blue |
| Connecting | amber | amber (breathing) |
| Waiting for NTP | magenta | magenta (breathing) |
| Running OK | — (clock shown) | dim green |
| Wi-Fi lost | brief red flashes over the clock | red |
| OTA update | white chase | white (fast blink) |

*(Blue means **setup mode**, not "connected" — green is the running color.)*

---

## Power / brightness notes

The firmware estimates strip current every frame and scales brightness down if a scene would
exceed **`ledBudgetMa`** (default **2400 mA**, editable in the UI up to 4000 mA). With the 5 V / 3 A
supply the clock face runs at full brightness; whole-face animations are auto-dimmed to fit.
Defaults: brightness 64/255 (25%). Raise both only to match a larger supply.

## Notes / follow-ups

- Hourly **chime** is stubbed (a reserved setting + a hook in `Animator::start`); add when ready.
- The gradient sub-modes are intentionally provisional — audition them on the real panel and we'll
  tune/prune.
- The reference build drives the strip **direct** (GPIO1 → 330 Ω → DIN, no level shifter). If a
  *long* data run ever glitches at 3.3 V, a 74AHCT125 push-pull buffer is the optional fallback.
