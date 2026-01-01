#ifndef POTS_AND_BUTTONS_H_
#define POTS_AND_BUTTONS_H_

#include <pico/stdlib.h>
#include <brain-ui/pots.h>
#include <brain-ui/button.h>
#include <brain-common/brain-common.h>

/**
 * @brief Component class for managing potentiometers and buttons
 *
 * Handles reading potentiometer values and button states for the Brain board.
 * Provides a clean interface for the diagnostics system to query input state.
 */
class PotsAndButtons {
public:
	/**
	 * @brief Construct a new PotsAndButtons object
	 */
	PotsAndButtons();

	/**
	 * @brief Initialize potentiometers and buttons
	 *
	 * Sets up hardware for all 3 potentiometers and 2 buttons
	 */
	void init();

	/**
	 * @brief Update potentiometer and button states
	 *
	 * Call this regularly in the main loop to poll inputs (non-blocking)
	 */
	void update();

	/**
	 * @brief Get potentiometer value scaled to 0-127 range
	 *
	 * @param pot_index Potentiometer index (0-2)
	 * @return uint16_t Value from 0-127
	 */
	uint16_t get_pot_value(uint8_t pot_index);

	/**
	 * @brief Map potentiometer value to number of LEDs (0-6)
	 *
	 * Maps the pot range (0-127) to LED count:
	 * - 0% (0) = 0 LEDs
	 * - 100% (127) = 6 LEDs
	 *
	 * @param pot_index Potentiometer index (0-2)
	 * @return uint8_t Number of LEDs to light (0-6)
	 */
	uint8_t map_pot_to_leds(uint8_t pot_index);

	/**
	 * @brief Check if button 1 is currently pressed
	 *
	 * @return true if button 1 is pressed
	 */
	bool is_button1_pressed();

	/**
	 * @brief Check if button 2 is currently pressed
	 *
	 * @return true if button 2 is pressed
	 */
	bool is_button2_pressed();

	/**
	 * @brief Check if any button is currently pressed
	 *
	 * @return true if either button is pressed
	 */
	bool is_any_button_pressed();

private:
	brain::ui::Pots pots_;
	brain::ui::Button button1_;
	brain::ui::Button button2_;

	// Button state tracking
	bool button1_pressed_;
	bool button2_pressed_;

	// Pot value cache
	uint16_t pot_values_[3];
};

#endif  // POTS_AND_BUTTONS_H_
