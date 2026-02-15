/**
 * @file pots_and_buttons.cpp
 * @brief Component class for managing potentiometers and buttons
 *
 * Wraps Brain SDK's Pots and Button classes to provide a unified interface
 * for reading all 3 potentiometers and 2 buttons. Handles value caching,
 * button state tracking via callbacks, and LED mapping.
 *
 * Design notes:
 * - Uses Brain SDK's built-in debouncing for buttons
 * - Caches pot values for quick access without repeated ADC reads
 * - All operations are non-blocking
 */

#include "pots_and_buttons.h"
#include <stdio.h>

PotsAndButtons::PotsAndButtons()
	: button1_(BRAIN_BUTTON_1),
	  button2_(BRAIN_BUTTON_2),
	  button1_pressed_(false),
	  button2_pressed_(false) {
	// Initialize pot values to 0
	for (uint8_t i = 0; i < 3; i++) {
		pot_values_[i] = 0;
		last_reported_pot_values_[i] = 0;
	}
}

void PotsAndButtons::init() {
	printf("Initializing potentiometers and buttons...\n");

	// Initialize potentiometers with default config (3 pots, 7-bit resolution = 0-127)
	brain::ui::PotsConfig pot_config = brain::ui::create_default_config(3, 7);
	pots_.init(pot_config);

	// Initialize buttons (pull-up mode, button connects to GND)
	button1_.init(true);
	button2_.init(true);

	// Set up button press callbacks to track state
	button1_.set_on_press([this]() {
		button1_pressed_ = true;
		printf("Button 1 pressed\n");
	});

	button1_.set_on_release([this]() {
		button1_pressed_ = false;
		printf("Button 1 released\n");
	});

	button2_.set_on_press([this]() {
		button2_pressed_ = true;
		printf("Button 2 pressed\n");
	});

	button2_.set_on_release([this]() {
		button2_pressed_ = false;
		printf("Button 2 released\n");
	});

	printf("Potentiometers and buttons initialized\n");
}

void PotsAndButtons::update() {
	// Update button states (non-blocking)
	button1_.update();
	button2_.update();

	// Scan potentiometers for changes (non-blocking)
	pots_.scan();

	// Cache pot values for quick access and report significant changes
	for (uint8_t i = 0; i < 3; i++) {
		uint16_t new_value = pots_.get(i);
		pot_values_[i] = new_value;

		// Report significant changes in pot values
		int16_t change = new_value - last_reported_pot_values_[i];
		if (change < 0) change = -change;  // Absolute value

		if (change >= POT_CHANGE_THRESHOLD) {
			printf("Pot %d: %d (LEDs: %d/6)\n", i + 1, new_value, map_pot_to_leds(i));
			last_reported_pot_values_[i] = new_value;
		}
	}
}

uint16_t PotsAndButtons::get_pot_value(uint8_t pot_index) {
	if (pot_index < 3) {
		return pot_values_[pot_index];
	}
	return 0;
}

uint8_t PotsAndButtons::map_pot_to_leds(uint8_t pot_index) {
	if (pot_index >= 3) {
		return 0;
	}

	uint16_t pot_value = pot_values_[pot_index];

	// Map 0-127 to 0-6 LEDs
	// Add half the divisor for proper rounding
	uint8_t num_leds = (pot_value * 6 + 63) / 127;

	// Clamp to 0-6 range
	if (num_leds > 6) {
		num_leds = 6;
	}

	return num_leds;
}

bool PotsAndButtons::is_button1_pressed() {
	return button1_pressed_;
}

bool PotsAndButtons::is_button2_pressed() {
	return button2_pressed_;
}

bool PotsAndButtons::is_any_button_pressed() {
	return button1_pressed_ || button2_pressed_;
}
