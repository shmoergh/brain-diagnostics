#ifndef OUTPUTS_H_
#define OUTPUTS_H_

#include <pico/stdlib.h>
#include <brain-io/audio-cv-out.h>
#include <brain-io/pulse-output.h>

/**
 * @brief Component class for managing audio/CV and pulse outputs
 *
 * Handles signal generation for audio/CV outputs (2 channels) and pulse output.
 * Generates test waveforms (1Hz triangle for audio, 1Hz square for pulse).
 */
class Outputs {
public:
	/**
	 * @brief Output selection options
	 */
	enum class SelectedOutput {
		NONE = 0,
		AUDIO_A = 1,
		AUDIO_B = 2,
		PULSE = 3
	};

	/**
	 * @brief Construct a new Outputs object
	 *
	 * @param pulse_output Dedicated pulse output instance for writing pulse output
	 */
	Outputs(brain::io::PulseOutput* pulse_output);

	/**
	 * @brief Initialize audio/CV outputs
	 *
	 * Sets up DAC for audio outputs (pulse is initialized externally)
	 */
	void init();

	/**
	 * @brief Update output waveforms
	 *
	 * Call this regularly in the main loop to generate waveforms (non-blocking)
	 */
	void update();

	/**
	 * @brief Set which output is currently selected for testing
	 *
	 * @param selected The output to test
	 * @param print_change Whether to print selection change message (default true)
	 */
	void set_selected_output(SelectedOutput selected, bool print_change = true);

	/**
	 * @brief Get currently selected output
	 *
	 * @return SelectedOutput Currently selected output
	 */
	SelectedOutput get_selected_output() const;

	/**
	 * @brief Map pot value (0-127) to output selection
	 *
	 * Maps pot ranges:
	 * - 0 (0%) = NONE
	 * - 1-42 (0-33%) = AUDIO_A
	 * - 43-84 (33-66%) = AUDIO_B
	 * - 85-127 (66-100%) = PULSE
	 *
	 * @param pot_value Pot value from 0-127
	 * @return SelectedOutput The selected output based on pot value
	 */
	SelectedOutput map_pot_to_output_selection(uint16_t pot_value);

	/**
	 * @brief Set AC/DC coupling for audio outputs
	 *
	 * @param use_ac_coupling true for AC coupling, false for DC coupling
	 */
	void set_ac_coupling(bool use_ac_coupling);

	/**
	 * @brief Get current coupling mode
	 *
	 * @return true if AC coupling, false if DC coupling
	 */
	bool is_ac_coupled() const;

	/**
	 * @brief Get number of LEDs to show for output selection indicator
	 *
	 * Maps selected output to LED count:
	 * - NONE = 0 LEDs
	 * - AUDIO_A = 2 LEDs
	 * - AUDIO_B = 4 LEDs
	 * - PULSE = 6 LEDs
	 *
	 * @return uint8_t Number of LEDs (0, 2, 4, or 6)
	 */
	uint8_t get_selection_indicator_leds();

private:
	brain::io::AudioCvOut audio_cv_out_;
	brain::io::PulseOutput* pulse_output_;  // Pointer to dedicated pulse output instance

	SelectedOutput selected_output_;
	bool ac_coupled_;

	// Waveform generation state
	absolute_time_t waveform_start_time_;
	uint32_t last_phase_ms_;  // Track phase in milliseconds for waveform generation
	bool pulse_state_;  // Current state for square wave

	// Diagnostic output tracking
	uint32_t last_voltage_print_ms_;  // Track when we last printed voltage

	// Waveform parameters
	static constexpr uint32_t WAVEFORM_PERIOD_MS = 1000;  // 1Hz = 1000ms period
	static constexpr float MIN_VOLTAGE = 0.0f;
	static constexpr float MAX_VOLTAGE = 10.0f;
	static constexpr uint32_t VOLTAGE_PRINT_INTERVAL_MS = 100;  // Print voltage every 100ms

	// Helper methods
	void generate_triangle_wave(brain::io::AudioCvOutChannel channel);
	void generate_square_wave();
	void stop_all_outputs();
};

#endif  // OUTPUTS_H_
