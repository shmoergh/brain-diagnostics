#include <cstdint>

#include "pico/stdlib.h"

#include "tests.h"

namespace {

constexpr uint32_t kIndicatorDurationMs = 800;

Brain     g_brain;
TestId    g_current_test       = kTestLeds;
uint32_t  g_indicator_until_ms = 0;

uint32_t now_ms() {
    return to_ms_since_boot(get_absolute_time());
}

void show_binary(uint8_t value) {
    // LED 1 (index 0) is on the left of the board and acts as the MSB,
    // so a 6-bit binary number reads naturally left-to-right.
    for (uint8_t i = 0; i < 6; ++i) {
        uint8_t bit = 5 - i;
        if ((value >> bit) & 1u) {
            g_brain.leds.set_brightness(i, 255);
        } else {
            g_brain.leds.set_brightness(i, 0);
        }
    }
}

void advance_test() {
    g_current_test = static_cast<TestId>((g_current_test + 1) % kTestCount);
    on_test_enter(g_brain, g_current_test);
    g_indicator_until_ms = now_ms() + kIndicatorDurationMs;
}

}  // namespace

int main() {
    stdio_init_all();

    if (g_brain.init_all() == BrainInitStatus::kFailed) {
        // Init failure: blink button LED forever as a distress signal.
        g_brain.leds.button_start_blink(100);
        while (true) {
            g_brain.update();
            sleep_ms(10);
        }
    }

    // Try to load CV calibration from flash. If absent or corrupt, the
    // CV-output tests still run; the scope just sees the uncalibrated DAC
    // mapping. The diagnostics firmware never writes or clears calibration.
    g_brain.outputs.load_calibration_from_flash();

    g_brain.buttons.button_a.set_on_press(advance_test);
    g_brain.buttons.button_b.set_on_press(button_b_press);
    g_brain.buttons.button_b.set_on_release(button_b_release);

    g_brain.midi_parser.set_omni(true);
    g_brain.midi_parser.set_note_on_callback(midi_note_on);
    g_brain.midi_parser.set_note_off_callback(midi_note_off);

    on_test_enter(g_brain, g_current_test);
    g_indicator_until_ms = now_ms() + kIndicatorDurationMs;

    while (true) {
        g_brain.update();
        g_brain.midi_parser.process_uart();

        uint32_t t = now_ms();
        if (t < g_indicator_until_ms) {
            show_binary(static_cast<uint8_t>(g_current_test + 1));
        } else {
            run_test(g_brain, g_current_test, t);
        }
    }
}
