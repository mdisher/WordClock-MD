# WordClock-MD

A modernized ESP32-S3 word clock — a from-scratch, non-blocking rewrite of
[johniak/word-clock](https://github.com/johniak/word-clock) for the **Arduino IDE** and the
**ESP32-S3 SuperMini (HW-747 v0.0.2)**, keeping the original 132-LED serpentine frame and English
letter template.

It tells the time in words ("IT IS TWENTY MINUTES PAST ELEVEN"), sets itself up over a phone with a
captive-portal web page, keeps time via NTP with automatic DST, and can be updated over the air.

![WordClock-MD reading "IT IS NINE O'CLOCK" — black PLA frame with clear letters](WordClock%20Photo/WordClock_BlackPLA_ClearLetters.JPG)

*The v1.0 build (black PLA frame, clear letters) reading "IT IS NINE O'CLOCK." Note the bright LED
centers on the clear letters — [printing the letters in white](#the-physical-frame-3d-print)
diffuses this evenly.*

---

## Project status — v1.0

| | |
|---|---|
| Firmware | ✅ Complete, compiles clean (55% of an OTA partition), all modules implemented |
| Flashed & provisioned | ✅ Running on the ESP32-S3 SuperMini, on Wi-Fi at `wordclock.local` |
| **Physical build** | ✅ **Built and telling time** — panel assembled, letter map verified |
| Data path | ✅ Direct GPIO1 → 330 Ω → strip DIN (no level shifter needed) |
| Known follow-ups | White letter face plate (clear diffuses a touch bright); optional UI tweaks; hourly chime |

This is the **v1.0** release: a working, wall-ready word clock. See
[Roadmap / follow-ups](#roadmap--follow-ups) for the cosmetic / nice-to-have list.

---

## Features

- **Non-blocking** throughout (millis-driven state machine — no `delay()`, no busy-loops)
- **Captive-portal Wi-Fi setup** with a self-contained, mobile-first web UI
- **Settings dashboard** + **OTA** firmware updates (over Wi-Fi)
- **NTP** time via a POSIX **TZ string** → automatic DST, with a **daily resync**; the ESP32 RTC
  keeps time through Wi-Fi drops
- **Three color modes**: random-per-word (re-rolled on the minute, no flicker), single color, and
  gradient (per-word static / per-word cycle / whole-face drifting rainbow)
- **On-the-hour animations** (Sparkle / Sweep / Rainbow), non-blocking; hourly chime stubbed
- **Random-letter status indicator** for Wi-Fi / NTP / OTA states, mirrored on the onboard LED
- **Firmware current clamp** — every frame's strip current is estimated and brightness is scaled to
  stay under a configurable mA budget, so it's safe on the fixed supply. Worst-case clock display
  is 31 LEDs.

---

## Repository layout

```
WordClock-Modernization/
├── README.md            ← this file (project master doc)
├── HARDWARE.md          ← BOM (checkbox list) + ASCII wiring diagram
├── WordClock-wiring.svg ← scalable/printable wiring diagram (open in a browser)
├── WordClock-MD/        ← the Arduino sketch (flash this)
│   ├── WordClock-MD.ino   App state machine + setup()/loop()
│   ├── Config.h / Pins.h  build constants + pin/grid geometry
│   ├── Settings.*         NVS-persisted settings (Preferences)
│   ├── ClockDisplayHAL.*  132-LED grid, word map, current-clamped show()
│   ├── PowerBudget.*      per-frame mA estimate + brightness clamp
│   ├── ColorEngine.*      the 3 color modes
│   ├── TimeManager.*      POSIX-TZ NTP + daily resync
│   ├── WiFiProvisioning.* STA connect + captive-portal AP + DNS
│   ├── WebPortal.* / WebPage.h  async server: dashboard, JSON API, OTA
│   ├── Animator.*         non-blocking on-the-hour effects
│   ├── StatusIndicator.*  random-letter status + onboard GPIO48 LED
│   ├── WordClock.*        time→words logic (ported), millis-driven
│   ├── SerialHelper.*     guarded serial logging @ 115200
│   └── README.md          firmware-focused readme
└── wordclock/           ← ORIGINAL johniak project (PlatformIO), reference only
```

---

## How it works

A single non-blocking `loop()` drives everything — there is no `delay()` anywhere. Each pass does
three things:

1. **Services the plumbing:** `WiFiProvisioning` (connect / reconnect / portal DNS), `WebPortal`
   (deferred reboots), and `StatusIndicator` (the onboard LED).
2. **Runs the app state machine:** `BOOT → CONNECTING → PORTAL → RUN` (see `WordClock-MD.ino`).
   With saved creds it tries STA for 15 s, then falls back to the captive portal; once connected it
   syncs NTP and enters `RUN`.
3. **Renders one frame** (~30 fps) through a fixed **priority ladder** — first match wins:

   | Priority | Condition | What draws the grid |
   |---|---|---|
   | 1 | OTA in progress | `StatusIndicator` white "chase" |
   | 2 | Diagnostic/test mode (dashboard) | the word(s) under test |
   | 3 | On-the-hour animation active | `Animator` (Sparkle/Sweep/Rainbow) |
   | 4 | Boot / portal / connecting / NTP-wait | `StatusIndicator` random-letter animation |
   | 5 | Normal | `WordClock` composes the time (+ any Wi-Fi-lost overlay) |

### The display pipeline (how a color reaches an LED)

`WordClock.compose()` asks `ColorEngine` for each lit word's color and writes them into
`ClockDisplayHAL`'s **logical, full-intensity framebuffer** (one `0x00RRGGBB` per LED). Then
`ClockDisplayHAL::show()`:

1. asks `PowerBudget` for the **highest brightness whose estimated current stays under the mA
   budget**,
2. scales every pixel by that brightness, and
3. pushes to the strip via Adafruit_NeoPixel.

Because this runs on **every** `show()`, the current clamp governs the clock face *and* full-screen
animations alike — the strip can never exceed the budget no matter what's drawn. (Adafruit's own
lossy `setBrightness()` is left wide open at 255; we do our own scaling for finer control.)

### Time → words

`TimeManager` starts SNTP with a POSIX TZ string (so DST is automatic — no manual offsets) and reads
the ESP32 RTC, which keeps ticking even if Wi-Fi drops. `WordClock` buckets the minute into
5-minute steps, chooses **PAST** (:05–:34) or **TO** (:35–:59, rolling the hour forward one), and
maps each word to inclusive LED ranges via `WORDS_TO_LEDS[]` on the serpentine grid. Colors are
re-rolled **only when the minute changes**, so RANDOM mode is stable (no per-second flicker) while
gradient modes still animate smoothly.

### Settings & web

`Settings` persists a single versioned POD struct to NVS via `Preferences` (a magic + version guard
means a bad/old blob falls back to defaults). `WebPortal` serves one self-contained page
(`WebPage.h` — no external CSS/JS/fonts, so it works with no internet in AP mode) for both the
captive-portal setup and the full dashboard, plus a small JSON API and OTA upload.

---

## Hardware (summary)

Full BOM and wiring are in [`HARDWARE.md`](HARDWARE.md). In brief:

- **Board:** ESP32-S3 SuperMini (HW-747 v0.0.2)
- **Strip:** 132 × WS2812B (11 rows × 12, serpentine — same frame as the original)
- **Data:** GPIO1 → 330 Ω → strip DIN, **direct 3.3 V (no level shifter)**
- **Onboard status LED:** WS2812 on GPIO48 (no wiring)
- **Power:** external **5 V / 3 A** → distribution → strip (heavy) + board 5 V pin (light);
  **~940 µF (2 × 470 µF) bulk cap** (+ optional 100 nF) at the strip input, common ground, inject
  5 V/GND at both ends.

The ESP32's 3.3 V data drives the strip directly — the first WS2812 re-shapes the signal for the
rest of the chain, and over the short lead here it's rock-solid. A 74AHCT125 push-pull buffer is the
belt-and-suspenders option only if you ever run a long data lead and see glitches.

> ### ⚠️ Two build lessons worth their own boxes
>
> **Keep the PCB antenna clear.** The SuperMini's antenna is the zigzag trace at the **end opposite
> the USB-C connector**. Any metal, copper pour, header socket, or wiring **underneath or beside**
> it (within ~1 cm) detunes it and kills Wi-Fi — the board boots and the LED lights fine (they need
> far less signal than the radio), but the AP goes invisible and it won't join in range. **Mount the
> board so the antenna end overhangs into clear air.** Since firmware updates go over OTA, the box
> doesn't even need a USB-C hole — power off the 5 V pin and you're free to orient it antenna-out.
>
> **Power the strip before (or with) the logic.** Never leave the ESP32 powered and driving data
> into a strip whose 5 V is off — the data pin back-feeds current through the first LED's protection
> diode and kills it (the 330 Ω helps but isn't a guarantee). In a two-supply rig, switch them
> together or power the strip first.

> **Don't** power the board from USB-C and the 5 V supply's board-pin at the same time (two sources
> fighting on one rail). Two *separate* supplies (USB for logic, 5 V brick for the strip) sharing a
> common ground is fine.

---

## The physical frame (3D print)

The clock's frame/grid is the
[MakerWorld "Word Clock" model](https://makerworld.com/en/models/686196-word-clock).

> 💡 **Print the letters in WHITE filament, not clear.** Clear/transparent letters let each LED
> shine through as a bright **center hot-spot**; white diffuses the light evenly across the whole
> letter face. (v1.0 was printed in clear — a white letter reprint is the one cosmetic fix left.)

---

## Build & flash (Arduino IDE)

1. **Boards Manager** → install **esp32 by Espressif Systems** (tested on core 3.3.10).
2. Select board **"ESP32S3 Dev Module"** with:
   - **USB CDC On Boot: Enabled** (Serial over USB-C)
   - **Partition Scheme:** an OTA-capable scheme, e.g. *"Minimal SPIFFS (1.9MB APP with OTA)"*
   - **PSRAM:** Disabled
3. **Library Manager** → install:
   - **Adafruit NeoPixel**
   - **ESP Async WebServer** (ESP32Async fork — provides `ESPAsyncWebServer.h` + `AsyncJson.h`)
   - **Async TCP** (ESP32Async fork)
   - **ArduinoJson** (v7)

   *(DNSServer, Preferences, ESPmDNS, WiFi, Update, `time.h` ship with the ESP32 core.)*
4. Open `WordClock-MD/WordClock-MD.ino` (keep all files in the folder together), pick the port,
   and **Upload**.

### Or with arduino-cli

```bash
# find the port (ESP32-S3 shows up as an "ESP32 Family Device")
arduino-cli board list

# compile + upload (swap in your port)
arduino-cli compile --upload -p /dev/cu.usbmodemXXXX \
  --fqbn esp32:esp32:esp32s3:PartitionScheme=min_spiffs,CDCOnBoot=cdc \
  WordClock-MD
```

To wipe saved Wi-Fi/settings and start clean (e.g. recovery), full-erase first, then upload:

```bash
# esptool ships inside the installed esp32 core
ESPTOOL=~/Library/Arduino15/packages/esp32/tools/esptool_py/*/esptool
$ESPTOOL --chip esp32s3 -p /dev/cu.usbmodemXXXX erase_flash
# ...then the compile --upload above. NOTE: erase_flash clears NVS, so
# timezone/brightness/etc. reset to defaults and it boots into the setup portal.
```

### Watching the serial log

The ESP32-S3's **native USB drops on every reset**, which kills a plain `cat`/monitor. Use a
resilient reader that reattaches:

```bash
while true; do
  [ -e /dev/cu.usbmodemXXXX ] && { stty -f /dev/cu.usbmodemXXXX 115200 raw -echo 2>/dev/null; \
    cat /dev/cu.usbmodemXXXX 2>/dev/null; }
  sleep 0.3
done
```

(or `arduino-cli monitor -p /dev/cu.usbmodemXXXX -c baudrate=115200` in a real interactive terminal.)

---

## First run / provisioning

1. First boot (no saved Wi-Fi): the grid shows scattered **blue** letters, onboard LED **blue** —
   this is setup mode. It raises an open AP **`WordClock-Setup`**.
2. Join it with a phone; the captive portal opens automatically (or browse to `http://192.168.4.1`).
3. **Wi-Fi** tab → *Scan* → pick your network (2.4 GHz — the ESP32 can't see 5 GHz) → password →
   **Connect & save**. The board saves the creds and reboots to join.
4. It syncs NTP (**magenta** letters briefly), then runs (**green** onboard LED). Reach the
   dashboard at **`http://wordclock.local`** or the IP shown on the status chip.

> If the board has saved creds but can't reach that network (e.g. moved out of range), it tries for
> 15 s and then re-raises the `WordClock-Setup` portal as a fallback — your creds are **not** erased.

## Dashboard

The whole UI is one self-contained, mobile-first page served straight off the device — no app, no
cloud, no external assets (so it works even in AP setup mode with no internet).

<p align="center">
  <img src="Dashboard%20Screenshots/IMG_3584.PNG" width="300" alt="Clock tab: brightness &amp; power with live current estimate, and color mode">
  &nbsp;&nbsp;
  <img src="Dashboard%20Screenshots/IMG_3585.PNG" width="300" alt="Clock tab: on-the-hour animation, time zone, and the diagnostics letter-map tester">
</p>

<p align="center"><em>Clock tab — left: brightness + current budget (with the live mA estimate and
auto-scaled effective brightness) and color mode; right: on-the-hour animation, time zone, and the
Diagnostics letter-map tester.</em></p>

- **Clock:** brightness, current budget (mA), color mode + options, on-the-hour animation (with
  *Preview*), and timezone. Sliders preview live; **Save** persists to NVS.
- **Diagnostics:** light each word / cycle all / all-on to verify the letter map against the frame.
- **Wi-Fi:** re-provision the network.
- **System:** hostname, **OTA** firmware upload (`.bin`), factory reset.

### JSON API (for tinkering)

| Endpoint | Method | Purpose |
|---|---|---|
| `/api/status` | GET | live state (connection, time, est. mA, heap) |
| `/api/settings` | GET / POST | read / update settings (POST `{"save":1}` to persist) |
| `/api/scan` | GET | async Wi-Fi scan results |
| `/api/wifi` | POST | `{ssid,pass}` → save + reboot |
| `/api/test` | POST | `{mode:"single|cycle|all|off", index}` diagnostic |
| `/api/preview` | POST | play the configured animation now |
| `/api/ota` | POST | firmware upload (multipart) |
| `/api/reset` | POST | factory reset + reboot |

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

> Note: **blue means *setup mode*, not "connected."** Green is the "connected + running" color.

---

## Power / brightness

The firmware estimates strip current each frame (WS2812B model: ~20 mA/channel at full) and scales
brightness down if a scene would exceed **`ledBudgetMa`** (default **2400 mA**, editable up to 4000
in the UI). With the 5 V/3 A supply the clock face runs at full brightness; whole-face animations
are auto-dimmed to fit. Default brightness is 64/255 (25%). Raise either only to match a bigger
supply. Worst-case clock wording is **31 LEDs** ("IT IS TWENTYFIVE MINUTES PAST ELEVEN").

---

## OTA updates

Once it's on your network, you don't need USB again:

1. In Arduino IDE, **Sketch → Export Compiled Binary** (or `arduino-cli compile … --output-dir out`).
2. Dashboard → **System → Firmware update** → choose the `.bin` → **Upload & flash**. It reboots
   into the new firmware.

---

## Troubleshooting

- **No `WordClock-Setup` network / won't join Wi-Fi in range, but the board boots and the LED
  lights** — almost always the **PCB antenna is detuned** by nearby metal/wiring/headers. The radio
  needs far more clean signal than the CPU or LED. Reposition the board so the **antenna end (far
  from USB-C) is in clear air**; if it's seated in a socket/perfboard, make sure nothing conductive
  sits *under* that end. (A healthy portal prints `[WiFi] captive portal up: SSID "WordClock-Setup"`
  on serial.)
- **`wordclock.local` won't resolve** — native on macOS/iOS and Windows 10+; **Android** mDNS is
  spotty. Use the IP (shown on the dashboard chip and in the serial log / router client list).
- **Whole strip dark / a known-good driver won't light it** — the **first WS2812 is the usual
  casualty** (it fails to light *and* blocks data downstream). Confirm 5 V is present end-to-end,
  ensure your data source **shares ground** with the strip's supply and drives an **all-on** test
  pattern (not the mostly-off clock), then inject data at LED #1's DIN and walk down to find the
  first survivor. Fix: cut the dead head, resume from the first good LED. (See the power-before-data
  note above for how the first pixel usually dies.)
- **Strip: words scrambled but stable** — the LED map doesn't match your frame; adjust
  `WORDS_TO_LEDS[]` in `ClockDisplayHAL.cpp` (use Diagnostics → Cycle all to map it).
- **Upload: "Resource busy / could not open port"** — a **serial monitor** (often the Arduino IDE's)
  is holding the port. Close the Serial Monitor tab or quit the IDE, then upload.
- **Upload: "No serial data received / failed to connect to ESP32-S3"** — enter download mode:
  **hold `BOOT`, tap `RST`, release `BOOT`**, then upload. (The S3's native USB can miss the
  auto-reset window, especially during a crash loop.)
- **Boot loop / `assert failed: xQueueSemaphoreTake`** — the async web server was started before
  the Wi-Fi/lwIP stack. *(Fixed in this codebase: `setup()` brings Wi-Fi up before `web.begin()`.)*

---

## Roadmap / follow-ups

- **Reprint the letters in white** — v1.0 was printed with clear letters, which give each LED a
  bright center hot-spot; **white filament diffuses evenly.** Frame model:
  [MakerWorld Word Clock](https://makerworld.com/en/models/686196-word-clock). (Print swap, no code.)
- **Audition the gradient sub-modes** on the real panel and tune/prune (per-word static, per-word
  cycle, whole-face drift — the last one is the "does it look good?" wildcard).
- **Minor dashboard/UI tweaks** — planned polish pass.
- **Hourly chime** — reserved setting + a hook in `Animator::start()`; wire up when ready.

## License & credits

The **WordClock-MD firmware** (`WordClock-MD/`) is © 2026 Matt Disher and licensed
**GPL-3.0-or-later** — see [`LICENSE`](LICENSE).

Credit where it's due: the physical frame, serpentine layout, and English word mapping originate
from [johniak/word-clock](https://github.com/johniak/word-clock), included **unmodified** under
`wordclock/` for reference. That original repo carries **no license** (all rights reserved by its
author), so this repository's GPL-3.0 covers **only** the WordClock-MD firmware, not `wordclock/`.
Full attribution and third-party notices are in [`CREDITS.md`](CREDITS.md).
