# Brain Diagnostics

A manual hardware diagnostic firmware for the Brain Eurorack board. It supports both Brain variants — the original Pico 1 (RP2040) and the Pico 2 (RP2350). You flash this onto a freshly assembled Brain, plug it into USB, and then walk through fourteen tests one at a time by pressing a single button. There are no automated checks: the firmware drives the hardware in a known way, and you watch the LEDs and the oscilloscope to decide whether each component is healthy.

If you've just soldered together a Brain board and want to make sure every input, output, pot, button, and LED actually works before you move on to flashing your real firmware, this is the tool for that.

## Install

Prebuilt UF2 files live at the top level of this repo, one per platform:

- [`brain-diagnostics-pico.uf2`](brain-diagnostics-pico.uf2) — for the original Brain (Pico 1 / RP2040)
- [`brain-diagnostics-pico-2.uf2`](brain-diagnostics-pico-2.uf2) — for the Brain 2.0 (Pico 2 / RP2350)

To install:

1. Hold the **BOOTSEL** button on the Pico while plugging it into your computer over USB.
2. The board mounts as a USB drive (`RPI-RP2` for Pico 1, `RP2350` for Pico 2).
3. Drag the matching UF2 file onto that drive.
4. The drive disappears as soon as the copy completes — at that point the firmware is running. You don't need to power-cycle.

That's it. If you'd rather build from source, see the [Development](#development) section at the end.

## What you'll need

- The Brain board itself, fully assembled.
- A USB cable (the Pico's micro-USB or USB-C, depending on your variant).
- An **oscilloscope** for the CV output and pulse output tests. A cheap entry-level USB scope is plenty — the signals you're looking for are slow and not picky.
- A **MIDI controller** (any keyboard or pad device with a 5-pin DIN MIDI out and a MIDI-TRS-A converter) for the MIDI input test.
- A **CV source** for the CV input tests. A VCO from the Moduleur is ideal because it gives you a known repeating signal, but anything that produces a varying voltage in the ±5V range will do — an LFO, a sequencer, even a slowly-turned manual CV.
- A **clock or gate source** for the pulse input test. Any module that puts out a square gate signal works fine.
- Optional but useful: patch cables to do **loopback** tests — for instance, patching CV-out 1 into CV-in 1 lets you drive the CV input test from the firmware's own square wave rather than needing an external source.

## How to use it — the core idea

There is exactly one button you need to think about: **Button A**. Each press advances to the next test in a fixed loop of 13 tests. After the last one, the next press wraps you back around to the first. That's the whole interaction model. There's no menu, no terminal interaction, no serial commands, no chord shortcuts. You press Button A to step forward; the LEDs tell you everything else.

When you advance to a new test, the LED strip briefly flashes the test number in **binary** so you know where you are. After about 0.8 seconds the indicator clears and the test takes over the LED strip for its own feedback (a VU meter, a blink pattern, an on/off response, etc.).

Button A itself doesn't need its own dedicated test — the simple fact that pressing it cycles you through the tests is proof that it works.

## Reading the binary indicator

The six panel LEDs encode the test number. **LED 1 (leftmost) is the most significant bit** and **LED 6 (rightmost) is the least significant bit**, so you can read the LED strip left-to-right as a 6-bit binary number. The mapping for all 14 tests:

| Test | # | Binary (LED 1 → LED 6) | LEDs lit |
|------|---|------------------------|----------|
| LEDs | 1 | `000001` | LED 6 |
| Pot 1 | 2 | `000010` | LED 5 |
| Pot 2 | 3 | `000011` | LED 5, 6 |
| Pot 3 | 4 | `000100` | LED 4 |
| Button LED | 5 | `000101` | LED 4, 6 |
| Button B | 6 | `000110` | LED 4, 5 |
| MIDI input | 7 | `000111` | LED 4, 5, 6 |
| CV input 1 | 8 | `001000` | LED 3 |
| CV input 2 | 9 | `001001` | LED 3, 6 |
| Pulse input | 10 | `001010` | LED 3, 5 |
| CV output 1 | 11 | `001011` | LED 3, 5, 6 |
| CV output 2 | 12 | `001100` | LED 3, 4 |
| Pulse output | 13 | `001101` | LED 3, 4, 6 |
| CV out trimmer | 14 | `001110` | LED 3, 4, 5 |

So, for example: if you press Button A and see **LEDs 3 and 6** lit, reading left-to-right that's binary `001001` = 9, which means you're now on the **CV input 2** test. After ~0.8 seconds those LEDs go dark and the test takes over.

## The tests

The order is fixed. You can't jump around — Button A only ever moves forward. (When you wrap past test 13, the next press takes you back to test 1.)

### Test 1 — LEDs

All six panel LEDs gently breathe in and out together, fading from off to full brightness and back over about two seconds. Watch each LED individually. If any one of them stays dark, stays full-on, lags behind, or doesn't dim smoothly, that LED or its current-limit resistor is suspect. This test exercises PWM drive, so a "always on at full brightness" failure typically points at the GPIO not switching, not the LED itself.

### Tests 2, 3, 4 — Pot 1, Pot 2, Pot 3

For each pot, turn it fully counter-clockwise: the LED strip should be entirely dark. Slowly turn it clockwise; LEDs should light up one by one until all six are on at the maximum position. Sweep it back and forth a couple of times to confirm there are no dead spots in the pot's travel — a healthy pot gives you a smooth progression with no jumps or flickers between LED count thresholds. If a pot stays at zero or pinned at maximum no matter what, suspect a wiring issue between the pot and the multiplexer, or the pot itself.

### Test 5 — Button LED

The button's own integrated LED should blink steadily at about 4 Hz (250 ms on, 250 ms off). Nothing else lights up — the panel LED strip stays dark. If the button LED doesn't blink, or stays on/off solid, suspect either the LED itself or the GPIO driving it.

### Test 6 — Button B

The LED strip should be dark when nothing is pressed. Press and hold Button B and all six LEDs come on. Release and they go out. Tap it a few times to confirm the response is crisp and there's no bouncing or sticking. If the LEDs flicker on tap or the button feels "double-pressy", the SDK debounce should normally handle that — anything still misbehaving suggests a marginal switch.

### Test 7 — MIDI input

Plug in a MIDI controller via the Brain's MIDI input jack. Play a note on any channel. The LED strip should light up — all six LEDs on — while the note is held, and go dark when you release it. Try a chord: as long as **at least one note is sounding**, the LEDs should stay on; release notes one at a time and the LEDs should only go dark when the last note is released.

The firmware listens in **omni mode**, meaning it accepts MIDI from every channel (1–16). You don't need to set your controller to any particular channel.

If the LEDs never respond to MIDI, the most likely culprits are the MIDI input opto-isolator, the UART RX wiring, or the 5-pin DIN jack itself. A multimeter check of continuity from the DIN pins to the opto-isolator's input is a quick first step.

### Tests 8, 9 — CV input 1, CV input 2

Patch a VCO or any other CV source into the corresponding input jack. The LED strip behaves like a **VU meter**: more LEDs light up as the absolute value of the input signal moves further away from 0V. The input is configured for the ±5V range (which is the standard Brain SDK setting), so any typical Eurorack signal will give you a clean reading. A VCO sweeping a few octaves makes the LEDs dance visibly.

If the strip stays completely dark with a known-live signal patched in, suspect the input op-amp, a missing or shorted protection diode, or the ADC routing on the Pico.

If you don't have an external CV source handy, you can do a loopback: run test 11 (CV output 1) first to confirm that output works on a scope, then patch CV-out 1 into CV-in 1 and switch to test 8 — the firmware's own ±5V square wave will drive the input meter.

### Test 10 — Pulse input

Patch a clock, gate, or trigger source into the pulse-in jack. The LED strip should be all-on while the input is logic-high and all-off while it's logic-low. A slow clock (1–4 Hz) is the easiest to watch; a fast clock will look like everything is on continuously.

A loopback alternative: switch to test 13 first (pulse output runs at 10 Hz), patch pulse-out into pulse-in, then come back to test 10. You should see the LEDs flicker rapidly in sync with the output.

### Tests 11, 12 — CV output 1, CV output 2

Connect an oscilloscope to the corresponding CV output jack. You should see a **100 Hz square wave** swinging from roughly **−5 V to +5 V**. If calibration was loaded successfully on boot, the peaks should sit very close to ±5 V; if there's no calibration on the board, the peaks may be off by a few tens of millivolts — that's expected and not a hardware fault, just a sign that the board hasn't been calibrated yet.

Things to look at on the scope: the high and low levels, the cleanliness of the edges (no excessive ringing or overshoot), and that the wave is actually toggling at 100 Hz. A flat line at 0V or pinned at one extreme suggests an SPI problem to the MCP4822 DAC, a stuck CD4053 coupling switch, or a bad solder joint in the output stage.

### Test 13 — Pulse output

Connect a scope (or a logic probe, or another module's pulse input) to the pulse-out jack. You should see a **10 Hz square wave at logic levels** — about 5 V high, 0 V low, 50 % duty cycle.

If you don't have a scope, the loopback trick works here too: patch pulse-out into pulse-in and switch to test 10; you'll see the LED strip flicker rapidly as the output toggles.

### Test 14 — CV output hardware-trimmer calibration

This test is for tuning the **hardware trimmer** on the Brain board — the analog gain pot that sets the absolute scale of the CV output stage. It's a separate concern from the per-board software calibration that lives in flash; the hardware trimmer gets the analog stage roughly right, and the software calibration (set up later via the [CV tuner firmware](https://github.com/shmoergh/brain-cv-tuner)) compensates for the residual drift. To make sure this test reflects the raw analog hardware, it deliberately bypasses software calibration — voltages are written straight to the DAC.

Both CV outputs are switched to the 0–10 V range. **Pot 1 controls CV out 1, Pot 2 controls CV out 2.** Each pot selects a whole-volt step from the set {0, 1, 2, 3, 4, 5} V — turning the pot left-to-right walks through these six values. The output is held at a constant DC voltage (no toggling).

The LED strip shows where each pot is sitting. The six LEDs represent the six voltage steps: LED 1 is 0 V, LED 2 is 1 V, on through LED 6 = 5 V. The currently selected step glows; if both pots happen to land on the same step, that LED is at full brightness, otherwise the contributing channel lights its step at half brightness. So a quick glance tells you which volt each channel is currently outputting without needing to look at the meter.

To actually trim the hardware: connect a multimeter (or scope set to DC) to one of the CV outputs and step the corresponding pot through 0 V → 1 V → 2 V → ... → 5 V. Adjust the trimmer until the **difference** between consecutive steps is as close to 1 V as you can get. You don't need each absolute reading to be perfectly correct — the absolute offset is what software calibration fixes — you only need the *spacing* to be 1 V per step. A common workflow is to set the pot to 0 V, note the multimeter reading, then set the pot to 5 V and aim for a reading exactly 5 V higher than the 0 V reading; nudge the trimmer, repeat, until the spread is right. Then sanity-check the intermediate steps.

Repeat for the other channel using its own trimmer.

#### After the trimmer: run software calibration

Once you've trimmed the hardware as close to 1 V per step as you can get, the next step is to run **software calibration**. The hardware trimmer sets the overall gain of the analog stage, but it can't compensate for non-linearities, op-amp offset, or per-step drift — those residuals are exactly what software calibration is for. Skipping this step means your CV outputs will still be a few tens of millivolts off at any given voltage, which is enough to throw off 1 V/octave pitch tracking.

Software calibration is handled by a separate firmware called the **[Brain CV tuner](https://github.com/shmoergh/brain-cv-tuner)**. The workflow is:

1. Flash the CV tuner firmware onto the Brain (drag-and-drop UF2, same as this firmware).
2. Follow the CV tuner's README — it walks you through measuring each whole-volt step on a multimeter, entering the readings, and writing the resulting calibration table to the Brain's reserved flash sector.
3. Once calibration is written, flash your real firmware back onto the board. The calibration sector is protected by the same flash-reservation mechanism this diagnostics firmware uses, so it survives subsequent firmware flashes as long as those firmwares also reserve it.

After software calibration, your Brain's CV outputs will hit the asked-for voltage to within a fraction of a millivolt across the full range — good enough for accurate pitch tracking and any other CV use you care about.

## What to do if a test fails

A failure on a single test usually points you at a specific component you can probe with a multimeter or a scope. A few common starting points:

- **Test 1 (LEDs) — one LED dead:** reflow that LED and its current-limit resistor. If it's still dead, swap the LED.
- **Tests 2–4 (Pots) — pot stuck at 0 or 127:** check the wiper-to-multiplexer trace, then the multiplexer's select lines (S0/S1) from the Pico.
- **Test 5 (Button LED) — no blink:** check the GPIO trace from the Pico to the LED's anode-side resistor. Probe the GPIO with a scope while the test runs — you should see a clean square wave.
- **Test 6 (Button B) — no response:** continuity-check the switch itself; if it's good, check the GPIO pull-up.
- **Test 7 (MIDI) — no response:** check the MIDI DIN jack solder joints first, then the opto-isolator, then the UART RX wiring.
- **Tests 8–9 (CV in) — no response:** scope the input op-amp's output to confirm the analog signal reaches the Pico's ADC pin.
- **Test 10 (Pulse in) — no response:** scope the GPIO directly while applying a known signal.
- **Tests 11–12 (CV out) — no signal at all:** suspect the SPI lines (SCK/MOSI/CS) to the MCP4822 first.
- **Test 13 (Pulse out) — no signal:** scope the GPIO directly. If the GPIO toggles but the jack doesn't, the output buffer or jack wiring is the issue.

If multiple tests fail in a related way (e.g. all CV is dead), suspect a shared rail like the analog supply or ground.

## Development

### Source layout

The firmware is intentionally short. There are exactly three source files:

- `main.cpp` — boots the Brain SDK, registers Button A / Button B / MIDI callbacks, runs the main loop, manages the binary test indicator.
- `tests.cpp` / `tests.h` — the 13 per-test handlers, plus the `on_test_enter()` reset logic.
- `CMakeLists.txt` — build configuration, including the all-important `brain_storage_configure_flash_reservation()` call.

To add a new test: append a new value to the `TestId` enum in `tests.h`, add a `case` for it in `run_test()` (and `on_test_enter()` if you need to reset state), and the binary indicator and Button A cycling logic will pick it up automatically. The current 14 tests fit easily in 4 bits, so you have room to grow up to 63 tests before the LED strip runs out of binary digits.

### Build from source

First time you check out the repo, pull the SDK submodule:

```bash
git submodule update --init --recursive
```

The easiest way to build is the `build-firmware.sh` helper, which produces both the Pico 1 and Pico 2 UF2 files in one go and copies them to the repo root:

```bash
./build-firmware.sh
```

That writes `brain-diagnostics-pico.uf2` and `brain-diagnostics-pico-2.uf2` next to this README, replacing the prebuilt ones. The intermediate build trees end up in `build-pico/` and `build-pico-2/` respectively.

To build a single variant manually instead:

```bash
# Pico 2 (default)
cmake -B build
cmake --build build

# Pico 1
cmake -B build-pico -DPICO_BOARD=pico -DPICO_PLATFORM=rp2040
cmake --build build-pico
```

Flashing is the same drag-and-drop procedure described in [Install](#install), just using the freshly built UF2 instead of the prebuilt one.

### Calibration is preserved across flashes

The Brain board stores its CV output calibration data in a reserved sector at the top of the Pico's flash memory. This calibration is what makes your CV outputs hit accurate, predictable voltages when you ask for, say, exactly +5V. **Losing it means you'd have to re-run the calibration procedure using the [CV tuner firmware](https://github.com/shmoergh/brain-cv-tuner).**

This diagnostics firmware is built so that flashing it does **not** disturb that calibration. There are two reasons for that:

1. The CMake build calls `brain_storage_configure_flash_reservation()` *before* `pico_sdk_init()`. That tells the linker to keep the firmware image out of the flash region where calibration lives. As a result, when you drag the UF2 onto the board, only the program area is overwritten and the calibration sector is left untouched.
2. The firmware itself never calls `write_cv_calibration` or `clear_cv_calibration`. It only ever *reads* calibration on boot via `load_calibration_from_flash()`, so the CV-output tests show calibrated voltages.

If you ever edit `CMakeLists.txt` and remove or disable the `brain_storage_configure_flash_reservation()` line, the next UF2 you flash from this project can quietly overwrite the calibration sector. Don't do that. If you're not sure whether your build is reserving flash correctly, look at the CMake configure output — you should see a line like `[brain-storage] Reserved 12288 bytes at top-of-flash.` (or `8192 bytes` on RP2040). If you don't, stop and figure out why before flashing.

### Updating the SDK

`brain-sdk/` is a git submodule pinned to a specific tag (currently `v2.0`). To bump it:

```bash
cd brain-sdk
git fetch --tags
git checkout <new-tag>
cd ..
git add brain-sdk
git commit -m "Bump brain-sdk to <new-tag>"
```

The Brain SDK is API-versioned and major version bumps are deliberately breaking changes — read the SDK's `CHANGELOG.md` and release notes before bumping. After a bump, do a clean build of both variants (`rm -rf build-pico build-pico-2 && ./build-firmware.sh`) to catch any API changes that broke compilation.
