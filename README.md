# brain-diagnostics

Diagnostics firmware for the Brain board (Raspberry Pi Pico + brain-sdk).

It validates:
- LEDs
- 3 pots and 2 buttons
- audio/CV inputs
- audio/CV outputs
- pulse input and pulse output

## Build

```bash
git submodule update --init --recursive
mkdir -p build
cd build
cmake ..
make -j
```

Outputs are generated in `build/`, including:
- `brain-diagnostics.uf2`
- `gpio-test.uf2` (minimal GPIO helper firmware)

## Flash

1. Hold `BOOTSEL` while connecting the Pico over USB.
2. Mount the `RPI-RP2` drive.
3. Copy `build/brain-diagnostics.uf2` to the drive.

## Serial Monitor

The firmware prints usage and live diagnostics over serial (USB/UART stdio enabled).

Use one of:

```bash
screen /dev/tty.usbmodem* 115200
```

```bash
minicom -D /dev/tty.usbmodem* -b 115200
```

## How Diagnostics Works

### 1) Startup LED checks

On boot:
- startup animation runs 3 times (1 second interval)
- brightness test runs per LED at 20/40/60/80/100% (500 ms steps)

After this, firmware enters interactive mode and prints control instructions.

### 2) Interactive controls

Normal mode (both buttons not held):
- If any button is pressed, all LEDs turn on.
- If no input test is selected, LED bar follows the pot with the highest value.
- If input test is selected, LEDs show input test data (VU or pulse state).
- Output generation always runs independently in the background.
- For `Output = AUDIO_A` or `AUDIO_B`, Pot 3 continuously controls output mode (no buttons required):
  - `DC triangle`
  - `AC triangle`
  - `Fixed 0V` through `Fixed 10V` (1V steps)

Selection mode (hold BOTH buttons):
- Pot 1 selects **input source**
- Pot 2 selects **output source**
- LEDs show a status map while held:
  - LED1 Input A
  - LED2 Input B
  - LED3 Pulse In
  - LED4 Output A
  - LED5 Output B
  - LED6 Pulse Out
- Releasing both buttons prints the selected configuration to serial.

Selection ranges for pot 1 / pot 2 (`0..127`, mapping logic):
- `0`: NONE
- `1..42`: AUDIO_A
- `43..84`: AUDIO_B
- `85..127`: PULSE

Note: in current selection mode implementation, input/output changes are applied only when pot value is `> 0`.

## What to Test (Procedure)

### Pulse input

1. Hold both buttons and set **Input = PULSE**.
2. Apply a pulse signal to pulse-in.
3. Verify:
   - LEDs switch all-on/all-off with input state.
   - Serial shows `[INPUT] Pulse state ...` transitions.

### Pulse output

1. Hold both buttons and set **Output = PULSE**.
2. Verify pulse-out on scope or logic analyzer.
3. Expected: 1 Hz square wave (500 ms high / 500 ms low).
4. Serial logs `[OUTPUT] Pulse output: HIGH/LOW ...`.

### Audio/CV inputs

1. Set **Input = AUDIO_A** or **AUDIO_B**.
2. Inject signal to selected input.
3. Verify LED VU meter response and `Raw ADC` serial output.

### Audio/CV outputs

1. Set **Output = AUDIO_A** or **AUDIO_B**.
2. Use Pot 3 to choose output mode (`DC triangle`, `AC triangle`, or fixed `0..10V`).
3. Verify generated output:
   - Triangle modes: 1 Hz triangle wave, 0-10V.
   - Fixed modes: constant stepped voltage in 1V increments.
4. During fixed modes, output is forced to DC behavior for CV calibration.

### Loopback test (recommended)

Patch output to input to test both chains at once:
- Pulse out -> Pulse in
- Audio out A/B -> Audio/CV in A/B

Then select matching input/output and verify LEDs + serial logs together.

## Development

`brain-sdk` is a git submodule. To update:

```bash
cd brain-sdk
git pull origin main
cd ..
git add brain-sdk
git commit -m "Update brain-sdk"
```
