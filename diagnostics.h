#ifndef DIAGNOSTICS_H_
#define DIAGNOSTICS_H_

#include <pico/stdlib.h>
#include <brain-ui/leds.h>

/**
 * @brief Main diagnostics application class for Brain board hardware testing
 *
 * This class manages the complete diagnostics workflow for testing all
 * hardware components of the Brain board, including LEDs, potentiometers,
 * buttons, audio/CV inputs, and outputs.
 */
class Diagnostics {
public:
	/**
	 * @brief Construct a new Diagnostics object
	 */
	Diagnostics();

	/**
	 * @brief Initialize the diagnostics system
	 *
	 * Sets up all hardware components and prepares for testing
	 */
	void init();

	/**
	 * @brief Main update loop for diagnostics
	 *
	 * Call this regularly in the main loop to update all components
	 */
	void update();

private:
	// LED testing methods
	void test_leds();
	void test_led_startup_animation();
	void test_led_brightness();

	// Helper methods
	void delay_ms(uint32_t ms);

	// Components
	brain::ui::Leds leds_;

	// State tracking
	enum class State {
		LED_STARTUP_ANIMATION,
		LED_BRIGHTNESS_TEST,
		INTERACTIVE_MODE
	};

	State current_state_;

	// LED brightness test state
	uint8_t current_led_;
	uint8_t current_brightness_step_;
	absolute_time_t last_update_time_;
	uint8_t startup_animation_count_;
};

#endif  // DIAGNOSTICS_H_
