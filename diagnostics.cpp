/**
 * @file diagnostics.cpp
 * @brief Main diagnostics application implementation for Brain board hardware testing
 *
 * This file implements the complete diagnostics workflow including:
 * - LED startup and brightness testing (Milestone 1)
 * - Potentiometer and button testing (Milestone 2)
 * - Audio/CV and pulse input testing with VU meter (Milestone 3)
 * - Audio/CV and pulse output testing with waveform generation (Milestone 4)
 *
 * The implementation uses a state machine for LED testing, then enters an
 * interactive mode where users can test any component by selecting it with
 * the buttons and potentiometers.
 *
 * Key design principles:
 * - Non-blocking: no sleep() functions, all timing uses get_absolute_time()
 * - Minimal floating point: only where required by SDK APIs
 * - Component-based architecture: separate classes for inputs/outputs/pots/buttons
 * - Optimized for original Pico (not Pico 2)
 */

#include "diagnostics.h"
#include <stdio.h>

// Brightness levels for testing (in percentage converted to 0-255 range)
constexpr uint8_t BRIGHTNESS_LEVELS[] = {
	51,   // 20% of 255
	102,  // 40% of 255
	153,  // 60% of 255
	204,  // 80% of 255
	255   // 100% of 255
};
constexpr uint8_t NUM_BRIGHTNESS_LEVELS = 5;

// Timing constants (in milliseconds)
constexpr uint32_t STARTUP_ANIMATION_DELAY = 1000;
constexpr uint32_t BRIGHTNESS_TEST_DELAY = 500;

Diagnostics::Diagnostics()
	: inputs_(&pulse_),
	  outputs_(&pulse_),
	  current_state_(State::LED_STARTUP_ANIMATION),
	  current_led_(0),
	  current_brightness_step_(0),
	  startup_animation_count_(0),
	  both_buttons_held_(false),
	  last_num_leds_(255),  // Invalid value to force first update
	  last_leds_all_on_(false) {
}

void Diagnostics::init() {
	printf("\n");
	printf("==============================================\n");
	printf("    Brain Board Diagnostics Firmware\n");
	printf("==============================================\n");
	printf("Version: 1.0\n");
	printf("Board: Brain (Raspberry Pi Pico)\n");
	printf("==============================================\n\n");
	printf("Initializing hardware...\n");

	// Initialize LEDs
	leds_.init();
	printf("LEDs initialized\n");

	// Initialize potentiometers and buttons
	pots_and_buttons_.init();

	// Initialize shared pulse instance
	pulse_.begin();
	printf("Pulse I/O initialized\n");

	// Initialize inputs (audio/CV and pulse)
	inputs_.init();

	// Initialize outputs (audio/CV and pulse)
	outputs_.init();

	// Start LED testing
	current_state_ = State::LED_STARTUP_ANIMATION;
	last_update_time_ = get_absolute_time();

	printf("Starting LED diagnostics...\n");
}

void Diagnostics::update() {
	// State machine for different test phases
	switch (current_state_) {
		case State::LED_STARTUP_ANIMATION:
			test_led_startup_animation();
			break;

		case State::LED_BRIGHTNESS_TEST:
			test_led_brightness();
			break;

		case State::INTERACTIVE_MODE:
			interactive_mode();
			break;
	}
}

void Diagnostics::test_led_startup_animation() {
	absolute_time_t current_time = get_absolute_time();
	uint64_t elapsed_ms = absolute_time_diff_us(last_update_time_, current_time) / 1000;

	if (elapsed_ms >= STARTUP_ANIMATION_DELAY) {
		printf("Running startup animation (iteration %d/3)\n", startup_animation_count_ + 1);
		leds_.startup_animation();
		startup_animation_count_++;

		if (startup_animation_count_ >= 3) {
			// Finished startup animations, move to brightness test
			printf("Startup animation complete\n");
			printf("Starting LED brightness test...\n");
			current_state_ = State::LED_BRIGHTNESS_TEST;
			current_led_ = 0;
			current_brightness_step_ = 0;
		}

		last_update_time_ = current_time;
	}
}

void Diagnostics::test_led_brightness() {
	absolute_time_t current_time = get_absolute_time();
	uint64_t elapsed_ms = absolute_time_diff_us(last_update_time_, current_time) / 1000;

	if (elapsed_ms >= BRIGHTNESS_TEST_DELAY) {
		uint8_t brightness_value = BRIGHTNESS_LEVELS[current_brightness_step_];
		uint8_t brightness_percent = (current_brightness_step_ + 1) * 20;

		printf("LED %d: Setting brightness to %d%% (%d/255)\n",
			   current_led_ + 1,
			   brightness_percent,
			   brightness_value);

		// Set brightness for current LED
		leds_.set_brightness(current_led_, brightness_value);

		// Move to next brightness level
		current_brightness_step_++;

		if (current_brightness_step_ >= NUM_BRIGHTNESS_LEVELS) {
			// Finished all brightness levels for this LED
			// Turn off this LED and move to next
			leds_.off(current_led_);
			current_brightness_step_ = 0;
			current_led_++;

			if (current_led_ >= brain::ui::NO_OF_LEDS) {
				// Finished all LEDs
				printf("LED brightness test complete\n");
				printf("LED diagnostics passed!\n");
				printf("\n==============================================\n");
				printf("Entering interactive mode...\n");
				printf("==============================================\n\n");
				printf("USAGE:\n");
				printf("- Turn pots to see LED feedback\n");
				printf("- Press buttons to light all LEDs\n");
				printf("- Hold BOTH buttons (no pots) to see STATUS DISPLAY:\n");
				printf("  LED 1=Input A, LED 2=Input B, LED 3=Pulse In\n");
				printf("  LED 4=Output A, LED 5=Output B, LED 6=Pulse Out\n");
				printf("- Hold BOTH buttons + turn pot 1 to select INPUT\n");
				printf("- Hold BOTH buttons + turn pot 2 to select OUTPUT\n");
				printf("- Hold BOTH buttons + turn pot 3 to select coupling (AC/DC)\n");
				printf("- Release buttons to confirm selection\n");
				printf("\nMonitor this serial output for detailed diagnostics!\n");
				printf("==============================================\n\n");

				// All LEDs off after testing
				leds_.off_all();

				current_state_ = State::INTERACTIVE_MODE;
			}
		}

		last_update_time_ = current_time;
	}
}

void Diagnostics::interactive_mode() {
	// Update all components (non-blocking)
	pots_and_buttons_.update();
	inputs_.update();
	outputs_.update();  // Update waveform generation

	// Check if both buttons are held
	bool both_held = pots_and_buttons_.is_button1_pressed() &&
	                 pots_and_buttons_.is_button2_pressed();

	if (both_held) {
		// Selection mode - use knobs to select input, output, or coupling
		// First knob = input selection
		// Second knob = output selection
		// Third knob = AC/DC coupling

		uint16_t pot0 = pots_and_buttons_.get_pot_value(0);
		uint16_t pot1 = pots_and_buttons_.get_pot_value(1);
		uint16_t pot2 = pots_and_buttons_.get_pot_value(2);

		// Update selections based on pot positions (all pots checked independently)
		// Don't print selection changes while buttons held - only print final config on release
		// First knob controls input selection
		if (pot0 > 0) {
			Inputs::SelectedInput selected = inputs_.map_pot_to_input_selection(pot0);
			inputs_.set_selected_input(selected, false);  // Don't print while selecting
		}

		// Second knob controls output selection
		if (pot1 > 0) {
			Outputs::SelectedOutput selected = outputs_.map_pot_to_output_selection(pot1);
			outputs_.set_selected_output(selected, false);  // Don't print while selecting
		}

		// Third knob controls AC/DC coupling
		if (pot2 > 5) {  // Small threshold to avoid noise
			update_coupling_from_pot();
		}

		// Always show status display when both buttons are held
		// This shows which inputs/outputs are currently active in real-time
		show_status_display();

		both_buttons_held_ = true;
	} else {
		// Check if we just released buttons after selection
		if (both_buttons_held_) {
			// Buttons just released - print current configuration
			printf("\n=== Configuration ===\n");
			printf("Input: %d, Output: %d, Coupling: %s\n",
			       (int)inputs_.get_selected_input(),
			       (int)outputs_.get_selected_output(),
			       outputs_.is_ac_coupled() ? "AC" : "DC");
			printf("====================\n");

			// Turn off all LEDs when buttons are released
			leds_.off_all();
			last_num_leds_ = 0;
			last_leds_all_on_ = false;

			both_buttons_held_ = false;
		}

		// Simple logic: Input controls LEDs, output controls waveform generation
		// They are completely independent

		if (inputs_.get_selected_input() != Inputs::SelectedInput::NONE) {
			// Input selected - show VU meter or pulse on LEDs
			handle_input_testing();
		} else {
			// No input selected - normal pot and button mode
			update_leds_from_pots_and_buttons();
		}

		// Output never affects LEDs - it only generates waveforms via outputs_.update()
		// which is called at the top of interactive_mode()
	}
}

void Diagnostics::update_leds_from_pots_and_buttons() {
	// Priority 1: If any button is pressed, light all LEDs
	if (pots_and_buttons_.is_any_button_pressed()) {
		// Only update if not already all on
		if (!last_leds_all_on_) {
			leds_.on_all();
			last_leds_all_on_ = true;
			last_num_leds_ = 255;  // Mark as special state
		}
		return;
	}

	// Priority 2: Show the potentiometer with the highest value
	// This gives good interactive feedback when user turns any pot
	uint8_t max_pot_index = 0;
	uint16_t max_pot_value = 0;

	for (uint8_t i = 0; i < 3; i++) {
		uint16_t pot_value = pots_and_buttons_.get_pot_value(i);
		if (pot_value > max_pot_value) {
			max_pot_value = pot_value;
			max_pot_index = i;
		}
	}

	// Map the highest pot value to LED count
	uint8_t num_leds = pots_and_buttons_.map_pot_to_leds(max_pot_index);

	// Only update LEDs if the count changed
	if (num_leds != last_num_leds_ || last_leds_all_on_) {
		// Update LEDs: light up num_leds LEDs from the left
		for (uint8_t i = 0; i < brain::ui::NO_OF_LEDS; i++) {
			if (i < num_leds) {
				leds_.on(i);
			} else {
				leds_.off(i);
			}
		}
		last_num_leds_ = num_leds;
		last_leds_all_on_ = false;
	}
}

void Diagnostics::handle_input_testing() {
	Inputs::SelectedInput selected = inputs_.get_selected_input();

	if (selected == Inputs::SelectedInput::AUDIO_A ||
	    selected == Inputs::SelectedInput::AUDIO_B) {
		// Show VU meter for audio inputs
		uint8_t vu_level = inputs_.get_vu_meter_level();

		// Only update LEDs if the VU level changed
		if (vu_level != last_num_leds_ || last_leds_all_on_) {
			// Update LEDs to show VU meter
			for (uint8_t i = 0; i < brain::ui::NO_OF_LEDS; i++) {
				if (i < vu_level) {
					leds_.on(i);
				} else {
					leds_.off(i);
				}
			}
			last_num_leds_ = vu_level;
			last_leds_all_on_ = false;
		}
	} else if (selected == Inputs::SelectedInput::PULSE) {
		// Show pulse state
		bool pulse_high = inputs_.is_pulse_high();

		// Only update if pulse state changed
		if (pulse_high != last_leds_all_on_) {
			if (pulse_high) {
				leds_.on_all();
				last_leds_all_on_ = true;
				last_num_leds_ = 255;  // Mark as special state
			} else {
				leds_.off_all();
				last_leds_all_on_ = false;
				last_num_leds_ = 0;
			}
		}
	}
}

void Diagnostics::update_coupling_from_pot() {
	// Get third pot value (0-127)
	uint16_t pot_value = pots_and_buttons_.get_pot_value(2);

	// Map to coupling: < 64 = DC, >= 64 = AC
	bool use_ac = (pot_value >= 64);

	outputs_.set_ac_coupling(use_ac);
}

void Diagnostics::show_status_display() {
	// Status display mode: show which inputs and outputs are currently active
	// LED mapping:
	// LED 0 (LED 1) → Input Audio A active
	// LED 1 (LED 2) → Input Audio B active
	// LED 2 (LED 3) → Pulse In active
	// LED 3 (LED 4) → Output Audio A active
	// LED 4 (LED 5) → Output Audio B active
	// LED 5 (LED 6) → Pulse Out active

	Inputs::SelectedInput selected_input = inputs_.get_selected_input();
	Outputs::SelectedOutput selected_output = outputs_.get_selected_output();

	// LED 0: Input Audio A
	if (selected_input == Inputs::SelectedInput::AUDIO_A) {
		leds_.on(0);
	} else {
		leds_.off(0);
	}

	// LED 1: Input Audio B
	if (selected_input == Inputs::SelectedInput::AUDIO_B) {
		leds_.on(1);
	} else {
		leds_.off(1);
	}

	// LED 2: Pulse In
	if (selected_input == Inputs::SelectedInput::PULSE) {
		leds_.on(2);
	} else {
		leds_.off(2);
	}

	// LED 3: Output Audio A
	if (selected_output == Outputs::SelectedOutput::AUDIO_A) {
		leds_.on(3);
	} else {
		leds_.off(3);
	}

	// LED 4: Output Audio B
	if (selected_output == Outputs::SelectedOutput::AUDIO_B) {
		leds_.on(4);
	} else {
		leds_.off(4);
	}

	// LED 5: Pulse Out
	if (selected_output == Outputs::SelectedOutput::PULSE) {
		leds_.on(5);
	} else {
		leds_.off(5);
	}
}
