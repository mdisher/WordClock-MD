# WordClock-MD — Hardware BOM & Wiring

Bill of materials and wiring for the WordClock-MD build.
Power target: dedicated **5 V / 3 A** external supply (never runs all 132 LEDs at full white).

> 📐 A scalable, printable version of the wiring diagram is in
> [`WordClock-wiring.svg`](WordClock-wiring.svg) (open in any browser). The ASCII version below is
> the same circuit.

> ✅ **The reference build runs the data line DIRECT — no level shifter.** GPIO1 → 330 Ω → strip
> DIN. The ESP32's 3.3 V data is re-shaped by the first WS2812 and drives the chain reliably over
> the short lead here. A 74AHCT125 buffer is listed only as an *optional* fallback for long runs.

---

## Wiring diagram

```
        ┌───────────────────────────┐
        │     5V ⎓ 3A  POWER SUPPLY  │   dedicated to the clock;
        │        +5V        GND      │   board USB-C = flashing/serial only
        └─────────┬──────────┬───────┘
                  │          │
             [3A–3.5A fuse]  │                  ◄ optional, on the +5V leg
                  │          │
     ┌────────────┴──────────┴────────────┐
     │   DISTRIBUTION  (Wago / terminals)  │
     │       +5V rail        GND rail      │
     └──┬────────┬──────────┬────────┬─────┘
        │        │          │        │
    (heavy)  (light)    (heavy)   (light)
   +5V→strip +5V→board  GND→strip GND→board
        │        │          │        │
        │        │          │        └───────► ESP32-S3  GND pin
        │        └──────────────────────────► ESP32-S3  5V pin  (board draws ~0.3A)
        │                   │
        │   ┌───────────────┴───────────────┐
        │   │ ≈940µF cap across +5V / GND    │   ◄ at the strip, damps inrush
        │   │  (2×470µF ‖, or a 1000µF)      │
        │   └────────────────────────────────┘
        │
   ┌────┴──────── WS2812B STRIP (132 LEDs = 11 rows × 12, serpentine) ────────┐
   │  +5V ●────────────────────────────────────────────────────────────● +5V │ ◄ inject
   │  GND ●────────────────────────────────────────────────────────────● GND │   BOTH ends
   │  DIN ◄── data in                                              DOUT ──►    │   (even V)
   └───────▲─────────────────────────────────────────────────────────────────┘
           │
        [330Ω]   inline data resistor (right at the strip DIN)
           │
   ESP32-S3 GPIO1 (data out) ─── DIRECT 3.3V data, NO level shifter

                                      GPIO48 = onboard status LED (no wiring needed)

   ★ COMMON GROUND: PSU GND = board GND = strip GND ★
```

*(Optional: a 74AHCT125 could sit between the 330 Ω and DIN — VCC=5V, GND=common, 1OE→GND — only
if a long data run ever misbehaves. The reference build omits it.)*

---

## Connection table

| From | To | Wire |
|---|---|---|
| PSU +5V → (fuse) → +5V rail | distribution | 18–20 AWG |
| PSU GND → GND rail | distribution | 18–20 AWG |
| +5V rail → strip +5V (start) **and** far end | strip (inject both ends) | 20–22 AWG |
| GND rail → strip GND (start) **and** far end | strip | 20–22 AWG |
| +5V rail → ESP32 **5V pin** | board | 24 AWG |
| GND rail → ESP32 **GND** | board | 24 AWG |
| ESP32 **GPIO1** → 330 Ω → strip **DIN** (direct, no shifter) | data | 26 AWG |
| ≈940 µF cap (2×470 µF ‖) across strip **+5V/GND** | at strip | — |

---

## Bill of materials

Check off what you have; source the rest. Qty is for one clock.

### Core (you already have these)

- [ ] **ESP32-S3 SuperMini (HW-747 v0.0.2)** — ×1 — the MCU
- [ ] **WS2812B LED strip, 74 LED/m** — 132 LEDs total (11 sections × 12), cut serpentine — the display

### Power

- [ ] **5 V / 3 A power supply** — ×1 — barrel-jack adapter is cleanest (pair with a female DC pigtail); a 5 V/3 A USB charger + USB-A breakout also works
- [ ] **Female DC barrel jack pigtail** — ×1 — only if using a barrel-jack supply (screw-terminal type is easiest)
- [ ] **Inline fuse holder + 3 A–3.5 A fuse** — ×1 — *optional, recommended*, on the +5 V leg
- [ ] **Bulk capacitor ≈1000 µF, ≥ 10 V** (e.g. 2 × 470 µF in parallel, as the reference build used) — ×1 — across +5 V/GND at the strip; tames inrush
- [ ] **Distribution: Wago 221 connectors (×4) or a small screw-terminal block** — splits +5 V and GND to strip + board

### Data / logic

- [ ] **330 Ω resistor** (330–470 Ω, ¼ W) — ×1 — inline right at the strip DIN, reduces ringing
- [ ] **74AHCT125** (or 74AHCT245) level shifter — ×0 — **OPTIONAL fallback only.** The reference build runs **direct** (GPIO1 → 330 Ω → DIN); add a '125 (with a 0.1 µF decoupling cap) only if a *long* data run glitches at 3.3 V

### Wire & connectors

- [ ] **18–20 AWG hookup wire** (red/black) — ~1 m each — PSU + strip power runs
- [ ] **24–26 AWG hookup wire** — assorted — board taps + data line
- [ ] **JST-SM 3-pin pigtails** — ×1–2 pair — *optional*, tidy strip connections
- [ ] **Small slide switch (rated ≥ 3 A)** — ×1 — *optional*, on the +5 V line

### Assembly (nice to have)

- [ ] **Perfboard / small protoboard** — ×1 — to mount the resistor + cap (and the '125 if you use one)
- [ ] **Solder + hookup for the SuperMini** — solder the board in with its **antenna end overhanging into clear air** (see note 6)
- [ ] **Heat-shrink tubing** — assorted — insulate power joints

---

## Notes

1. **Common ground is mandatory** — PSU, board, and strip must share GND, or the data signal has
   no reference and the strip glitches.
2. **Never power the board from two sources on one rail.** The heavy LED current goes to the strip
   directly (not through the board's USB-C or traces). When flashing via USB-C, disconnect the 5 V
   supply's board tap — or use **OTA** after the first flash and leave USB unplugged in normal use.
   (Two *separate* supplies — USB for logic, 5 V brick for the strip — sharing a common ground is
   fine.)
3. **Power injection at both ends** of the strip keeps the far LEDs from dimming/shifting red.
4. **No level shifter needed** — the reference build drives the strip **direct**: GPIO1 → 330 Ω →
   DIN. The ESP32's 3.3 V data is re-shaped by the first WS2812 and runs the chain reliably over the
   short lead here. A 74AHCT125 push-pull buffer is an *optional* fallback only if a long data run
   ever shows flicker/wrong colors.
5. **Power the strip before (or with) the logic.** Never leave the board powered and driving data
   into a strip whose 5 V is off — the data pin back-feeds through the first LED's protection diode
   and kills it (the 330 Ω helps but isn't a guarantee). This is the #1 way the first WS2812 dies.
6. **Keep the PCB antenna clear.** The SuperMini's antenna is the zigzag trace at the end **opposite
   the USB-C connector.** Any metal/copper/wiring/header socket within ~1 cm — including *underneath*
   it — detunes it and kills Wi-Fi (the board still boots and the LED lights). Mount the board so the
   antenna end overhangs into open air. Since updates go over OTA, the enclosure needs no USB-C hole.
7. **Brightness/current** — the firmware caps strip current at a configurable budget (default
   2400 mA) and auto-dims to stay under it, so the 3 A supply always keeps headroom. Raise the budget
   in the dashboard only if you move to a bigger supply.
