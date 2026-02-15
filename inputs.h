#ifndef INPUTS_H_
#define INPUTS_H_

#include <pico/stdlib.h>
#include <brain-io/audio-cv-in.h>
#include <brain-io/pulse.h>

/**
 * @brief Component class for managing audio/CV and pulse inputs
 *
 * Handles reading audio/CV inputs (2 channels) and pulse input for the Brain board.
 * Provides VU meter visualization and pulse detection for diagnostics.
 */
class Inputs {
public:
	/**
	 * @brief Input selection options
	 */
	enum class SelectedInput {
		NONE = 0,
		AUDIO_A = 1,
		AUDIO_B = 2,
		PULSE = 3
	};

	/**
	 * @brief Construct a new Inputs object
	 *
	 * @param pulse Shared pulse instance for reading pulse input
	 */
	Inputs(brain::io::Pulse* pulse);

	/**
	 * @brief Initialize audio/CV inputs
	 *
	 * Sets up ADC for audio inputs (pulse is initialized externally)
	 */
	void init();

	/**
	 * @brief Update input readings
	 *
	 * Call this regularly in the main loop to poll inputs (non-blocking)
	 */
	void update();

	/**
	 * @brief Set which input is currently selected for testing
	 *
	 * @param selected The input to test
	 * @param print_change Whether to print selection change message (default true)
	 */
	void set_selected_input(SelectedInput selected, bool print_change = true);

	/**
	 * @brief Get currently selected input
	 *
	 * @return SelectedInput Currently selected input
	 */
	SelectedInput get_selected_input() const;

	/**
	 * @brief Map pot value (0-127) to input selection
	 *
	 * Maps pot ranges:
	 * - 0 (0%) = NONE
	 * - 1-42 (0-33%) = AUDIO_A
	 * - 43-84 (33-66%) = AUDIO_B
	 * - 85-127 (66-100%) = PULSE
	 *
	 * @param pot_value Pot value from 0-127
	 * @return SelectedInput The selected input based on pot value
	 */
	SelectedInput map_pot_to_input_selection(uint16_t pot_value);

	/**
	 * @brief Get VU meter level for currently selected audio input
	 *
	 * Returns number of LEDs to light (0-6) based on audio signal level.
	 * Only valid when AUDIO_A or AUDIO_B is selected.
	 *
	 * @return uint8_t Number of LEDs (0-6)
	 */
	uint8_t get_vu_meter_level();

	/**
	 * @brief Check if pulse input is currently high
	 *
	 * @return true if pulse input is high
	 */
	bool is_pulse_high();

	/**
	 * @brief Get number of LEDs to show for input selection indicator
	 *
	 * Maps selected input to LED count:
	 * - NONE = 0 LEDs
	 * - AUDIO_A = 2 LEDs
	 * - AUDIO_B = 4 LEDs
	 * - PULSE = 6 LEDs
	 *
	 * @return uint8_t Number of LEDs (0, 2, 4, or 6)
	 */
	uint8_t get_selection_indicator_leds();

private:
	brain::io::AudioCvIn audio_cv_in_;
	brain::io::Pulse* pulse_;  // Pointer to shared pulse instance

	SelectedInput selected_input_;

	// VU meter state for smoothing
	uint16_t vu_peak_hold_;
	absolute_time_t vu_peak_time_;
	static constexpr uint32_t VU_PEAK_HOLD_MS = 100;  // Hold peaks for 100ms

	// Pulse state tracking for change detection
	bool last_pulse_state_;
	bool pulse_state_initialized_;

	// Helper methods
	uint8_t calculate_vu_level(uint16_t raw_adc);
};

#endif  // INPUTS_H_
