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
	: current_state_(State::LED_STARTUP_ANIMATION),
	  current_led_(0),
	  current_brightness_step_(0),
	  startup_animation_count_(0) {
}

void Diagnostics::init() {
	printf("Brain Diagnostics Firmware\n");
	printf("Initializing hardware...\n");

	// Initialize LEDs
	leds_.init();
	printf("LEDs initialized\n");

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
			// TODO: Will be implemented in later milestones
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
				printf("\nEntering interactive mode...\n");
				printf("(Interactive features will be added in later milestones)\n");

				// All LEDs off after testing
				leds_.off_all();

				current_state_ = State::INTERACTIVE_MODE;
			}
		}

		last_update_time_ = current_time;
	}
}

void Diagnostics::delay_ms(uint32_t ms) {
	sleep_ms(ms);
}
