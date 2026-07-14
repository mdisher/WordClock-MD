# WordClock-MD — Hardware BOM & Wiring

Bill of materials and wiring for the WordClock-MD build.
Power target: dedicated **5 V / 3 A** external supply (never runs all 132 LEDs at full white).

> 📐 A scalable, printable version of the wiring diagram is in
> [`WordClock-wiring.svg`](WordClock-wiring.svg) (open in any browser). The ASCII version below is
> the same circuit.

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
        │   │  1000µF cap across +5V / GND   │   ◄ at the strip, damps inrush
        │   └────────────────────────────────┘
        │
   ┌────┴──────── WS2812B STRIP (132 LEDs = 11 rows × 12, serpentine) ────────┐
   │  +5V ●────────────────────────────────────────────────────────────● +5V │ ◄ inject
   │  GND ●────────────────────────────────────────────────────────────● GND │   BOTH ends
   │  DIN ◄── data in                                              DOUT ──►    │   (even V)
   └───────▲─────────────────────────────────────────────────────────────────┘
           │
      [74AHCT125]   3.3V → 5V level shifter   (VCC = 5V, GND = common)
           ▲
        [330Ω]   inline data resistor
           │
   ESP32-S3 GPIO1 (data out)          GPIO48 = onboard status LED (no wiring needed)

   ★ COMMON GROUND: PSU GND = board GND = strip GND = level-shifter GND ★
```

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
| +5V rail → 74AHCT125 VCC; GND rail → its GND | shifter | 26 AWG |
| ESP32 **GPIO1** → 330 Ω → 74AHCT125 input (1A) | data | 26 AWG |
| 74AHCT125 output (1Y) → strip **DIN** | data | 26 AWG |
| 1000 µF cap across strip **+5V/GND** | at strip | — |

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
- [ ] **1000 µF electrolytic capacitor, ≥ 10 V** (10–16 V) — ×1 — across +5 V/GND at the strip; tames inrush
- [ ] **Distribution: Wago 221 connectors (×4) or a small screw-terminal block** — splits +5 V and GND to strip + board

### Data / logic

- [ ] **74AHCT125** (or 74AHCT245) level shifter — ×1 — 3.3 V→5 V on the data line; recommended for a 132-LED run. DIP package is breadboard/perfboard friendly
- [ ] **330 Ω resistor** (330–470 Ω, ¼ W) — ×1 — inline on DIN, reduces ringing
- [ ] **0.1 µF ceramic capacitor** — ×1 — *optional*, decoupling across the 74AHCT125 VCC/GND

### Wire & connectors

- [ ] **18–20 AWG hookup wire** (red/black) — ~1 m each — PSU + strip power runs
- [ ] **24–26 AWG hookup wire** — assorted — board taps + data/logic lines
- [ ] **JST-SM 3-pin pigtails** — ×1–2 pair — *optional*, tidy strip connections
- [ ] **Small slide switch (rated ≥ 3 A)** — ×1 — *optional*, on the +5 V line

### Assembly (nice to have)

- [ ] **Perfboard / small protoboard** — ×1 — to mount the 74AHCT125, resistor, cap
- [ ] **Pin headers / Dupont jumpers** — as needed — solderless connections to the SuperMini
- [ ] **Heat-shrink tubing** — assorted — insulate power joints

---

## Notes

1. **Common ground is mandatory** — PSU, board, strip, and level shifter must share GND, or the
   data signal has no reference and the strip glitches.
2. **Never power the board from two sources at once.** The heavy LED current goes to the strip
   directly (not through the board's USB-C or traces). When flashing via USB-C, disconnect the 5 V
   supply — or use **OTA** after the first flash and leave USB unplugged in normal operation.
3. **Power injection at both ends** of the strip keeps the far LEDs from dimming/shifting red.
4. **Level shifter is recommended, not strictly required** — short runs sometimes work at 3.3 V
   data, but a 132-LED panel is where a 74AHCT125 earns its place. If you skip it, wire GPIO1 → 330 Ω
   → DIN directly and test.
5. **Brightness/current** — the firmware caps strip current at a configurable budget (default
   2400 mA) and auto-dims to stay under it, so the 3 A supply always keeps headroom. Raise the budget
   in the dashboard only if you move to a bigger supply.
