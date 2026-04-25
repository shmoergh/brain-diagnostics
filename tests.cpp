#include "tests.h"

#include <cstdint>
#include <cstdlib>

namespace {

constexpr uint32_t kCvSquareHalfPeriodMs = 5;     // 100 Hz square on CV outs
constexpr uint32_t kPulseHalfPeriodMs    = 50;    // 10 Hz square on pulse out
constexpr uint32_t kButtonLedBlinkMs     = 250;   // ~2 Hz blink (toggle interval)
constexpr uint32_t kLedSweepHalfMs       = 1000;  // 1 s up + 1 s down = 2 s period
constexpr int32_t  kCvOutHighMv          =  5000;
constexpr int32_t  kCvOutLowMv           = -5000;
constexpr int32_t  kCvInFullScaleMv      =  5000;
constexpr uint16_t kPotFullScale         = 127;   // 7-bit default

// MIDI note tracking. Callbacks must be plain functions (the SDK uses
// function pointers, not std::function), so the counter lives in file scope.
volatile uint8_t g_midi_active_notes = 0;

// Button B latched state. Updated by callbacks registered in main, read by
// the button-B test. Always-on registration keeps the test handler stateless.
volatile bool g_button_b_pressed = false;

// Per-test timer state. Reset in on_test_enter().
uint32_t g_last_toggle_ms = 0;
bool     g_toggle_state   = false;

uint8_t pot_to_led_count(uint16_t pot_value) {
    // 0..127 -> 0..6. Each LED step ~21 pot units.
    uint32_t scaled = static_cast<uint32_t>(pot_value) * 6 / (kPotFullScale + 1);
    if (scaled > 6) scaled = 6;
    return static_cast<uint8_t>(scaled);
}

void light_bar(Brain& brain, uint8_t count) {
    for (uint8_t i = 0; i < 6; ++i) {
        if (i < count) {
            brain.leds.set_brightness(i, 255);
        } else {
            brain.leds.set_brightness(i, 0);
        }
    }
}

uint8_t triangle_brightness(uint32_t now_ms) {
    // 0 -> 255 -> 0 over 2 * kLedSweepHalfMs.
    uint32_t phase = now_ms % (2 * kLedSweepHalfMs);
    uint32_t up = (phase < kLedSweepHalfMs) ? phase
                                            : (2 * kLedSweepHalfMs - phase);
    return static_cast<uint8_t>((up * 255) / kLedSweepHalfMs);
}

}  // namespace

void midi_note_on(uint8_t /*note*/, uint8_t velocity, uint8_t /*channel*/) {
    // Running-status convention: note-on with velocity 0 means note-off.
    if (velocity == 0) {
        if (g_midi_active_notes > 0) g_midi_active_notes--;
    } else {
        if (g_midi_active_notes < 255) g_midi_active_notes++;
    }
}

void midi_note_off(uint8_t /*note*/, uint8_t /*velocity*/, uint8_t /*channel*/) {
    if (g_midi_active_notes > 0) g_midi_active_notes--;
}

void button_b_press()   { g_button_b_pressed = true; }
void button_b_release() { g_button_b_pressed = false; }

void on_test_enter(Brain& brain, TestId test) {
    // Reset shared state.
    brain.leds.off_all();
    brain.leds.button_stop_blink();
    for (uint8_t i = 0; i < 6; ++i) {
        brain.leds.stop_blink(i);
        brain.leds.set_brightness(i, 0);
    }
    g_last_toggle_ms = 0;
    g_toggle_state = false;
    g_midi_active_notes = 0;

    switch (test) {
        case kTestButtonLed:
            brain.leds.button_start_blink(kButtonLedBlinkMs);
            break;
        case kTestCvOut1:
            brain.outputs.set_output_range(kOutputsChannelA, kOutputsRangeMinus5To5V);
            break;
        case kTestCvOut2:
            brain.outputs.set_output_range(kOutputsChannelB, kOutputsRangeMinus5To5V);
            break;
        case kTestPulseOut:
            brain.outputs.pulse_set(false);
            break;
        default:
            break;
    }
}

void run_test(Brain& brain, TestId test, uint32_t now_ms) {
    switch (test) {
        case kTestLeds: {
            uint8_t b = triangle_brightness(now_ms);
            for (uint8_t i = 0; i < 6; ++i) {
                brain.leds.set_brightness(i, b);
            }
            break;
        }

        case kTestPot1:
            light_bar(brain, pot_to_led_count(brain.pots.get_buffered(0)));
            break;
        case kTestPot2:
            light_bar(brain, pot_to_led_count(brain.pots.get_buffered(1)));
            break;
        case kTestPot3:
            light_bar(brain, pot_to_led_count(brain.pots.get_buffered(2)));
            break;

        case kTestButtonLed:
            // Blink is driven by leds.update() in brain.update(); nothing to do here.
            break;

        case kTestButtonB:
            if (g_button_b_pressed) {
                for (uint8_t i = 0; i < 6; ++i) brain.leds.set_brightness(i, 255);
            } else {
                for (uint8_t i = 0; i < 6; ++i) brain.leds.set_brightness(i, 0);
            }
            break;

        case kTestMidi:
            if (g_midi_active_notes > 0) {
                for (uint8_t i = 0; i < 6; ++i) brain.leds.set_brightness(i, 255);
            } else {
                for (uint8_t i = 0; i < 6; ++i) brain.leds.set_brightness(i, 0);
            }
            break;

        case kTestCvIn1: {
            int32_t mv = brain.inputs.get_voltage_millivolts(kInputsChannelA);
            uint32_t mag = static_cast<uint32_t>(std::abs(mv));
            if (mag > static_cast<uint32_t>(kCvInFullScaleMv)) {
                mag = kCvInFullScaleMv;
            }
            uint8_t count = static_cast<uint8_t>((mag * 6) / kCvInFullScaleMv);
            if (count > 6) count = 6;
            light_bar(brain, count);
            break;
        }
        case kTestCvIn2: {
            int32_t mv = brain.inputs.get_voltage_millivolts(kInputsChannelB);
            uint32_t mag = static_cast<uint32_t>(std::abs(mv));
            if (mag > static_cast<uint32_t>(kCvInFullScaleMv)) {
                mag = kCvInFullScaleMv;
            }
            uint8_t count = static_cast<uint8_t>((mag * 6) / kCvInFullScaleMv);
            if (count > 6) count = 6;
            light_bar(brain, count);
            break;
        }

        case kTestPulseIn:
            if (brain.inputs.pulse_read()) {
                for (uint8_t i = 0; i < 6; ++i) brain.leds.set_brightness(i, 255);
            } else {
                for (uint8_t i = 0; i < 6; ++i) brain.leds.set_brightness(i, 0);
            }
            break;

        case kTestCvOut1:
            if (now_ms - g_last_toggle_ms >= kCvSquareHalfPeriodMs) {
                g_last_toggle_ms = now_ms;
                g_toggle_state = !g_toggle_state;
                int32_t mv = g_toggle_state ? kCvOutHighMv : kCvOutLowMv;
                brain.outputs.set_voltage_calibrated_millivolts(kOutputsChannelA, mv);
            }
            break;
        case kTestCvOut2:
            if (now_ms - g_last_toggle_ms >= kCvSquareHalfPeriodMs) {
                g_last_toggle_ms = now_ms;
                g_toggle_state = !g_toggle_state;
                int32_t mv = g_toggle_state ? kCvOutHighMv : kCvOutLowMv;
                brain.outputs.set_voltage_calibrated_millivolts(kOutputsChannelB, mv);
            }
            break;

        case kTestPulseOut:
            if (now_ms - g_last_toggle_ms >= kPulseHalfPeriodMs) {
                g_last_toggle_ms = now_ms;
                g_toggle_state = !g_toggle_state;
                brain.outputs.pulse_set(g_toggle_state);
            }
            break;

        case kTestCount:
            break;
    }
}
