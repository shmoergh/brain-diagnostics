/**
 * @file outputs.cpp
 * @brief Component class for managing audio/CV and pulse outputs
 *
 * Wraps Brain SDK's AudioCvOut (DAC-based) and Pulse classes to provide
 * output testing functionality. Generates 1Hz test waveforms for verification.
 *
 * Waveform Generation:
 * - Triangle wave for audio/CV: 0-10V, 1Hz (chose triangle over sine for CPU efficiency)
 * - Square wave for pulse: 1Hz digital signal
 * - Time-based generation using get_absolute_time()
 * - Updates only when millisecond phase changes (reduces CPU load)
 * - Minimal floating point (only for voltage, required by DAC API)
 *
 * Design notes:
 * - AC/DC coupling controlled by third pot (< 50% = DC, >= 50% = AC)
 * - Requires holding both buttons to adjust coupling
 * - All operations are non-blocking
 * - Output selection uses pot value ranges (same as inputs)
 * - Can be used simultaneously with inputs for loopback testing
 */

#include "outputs.h"
#include <stdio.h>

Outputs::Outputs(brain::io::PulseOutput* pulse_output)
	: pulse_output_(pulse_output),
	  selected_output_(SelectedOutput::NONE),
	  ac_coupled_(false),
	  last_phase_ms_(0),
	  pulse_state_(false),
	  last_voltage_print_ms_(0) {
	waveform_start_time_ = get_absolute_time();
}

void Outputs::init() {
	printf("Initializing outputs...\n");

	// Initialize audio/CV outputs with default pins
	if (!audio_cv_out_.init()) {
		printf("WARNING: Failed to initialize audio/CV outputs\n");
	} else {
		printf("Audio/CV outputs initialized successfully\n");
	}

	// Pulse is initialized externally - set initial state
	pulse_output_->set(false);  // Start with pulse low
	printf("Pulse output ready (dedicated instance)\n");

	// Set default DC coupling
	audio_cv_out_.set_coupling(brain::io::AudioCvOutChannel::kChannelA,
	                            brain::io::AudioCvOutCoupling::kDcCoupled);
	audio_cv_out_.set_coupling(brain::io::AudioCvOutChannel::kChannelB,
	                            brain::io::AudioCvOutCoupling::kDcCoupled);
	printf("Default coupling: DC\n");

	printf("Outputs initialized\n");
}

void Outputs::update() {
	// Only generate waveforms if an output is selected
	if (selected_output_ == SelectedOutput::NONE) {
		return;
	}

	// Generate appropriate waveform based on selected output
	switch (selected_output_) {
		case SelectedOutput::AUDIO_A:
			generate_triangle_wave(brain::io::AudioCvOutChannel::kChannelA);
			break;

		case SelectedOutput::AUDIO_B:
			generate_triangle_wave(brain::io::AudioCvOutChannel::kChannelB);
			break;

		case SelectedOutput::PULSE:
			generate_square_wave();
			break;

		default:
			break;
	}
}

void Outputs::set_selected_output(SelectedOutput selected, bool print_change) {
	if (selected != selected_output_) {
		// Stop previous output
		stop_all_outputs();

		selected_output_ = selected;

		// Reset waveform timing
		waveform_start_time_ = get_absolute_time();
		last_phase_ms_ = 0;
		pulse_state_ = false;

		// Print selection change only if requested
		if (print_change) {
			const char* output_name;
			switch (selected) {
				case SelectedOutput::NONE:
					output_name = "NONE";
					break;
				case SelectedOutput::AUDIO_A:
					output_name = "AUDIO_A (1Hz triangle wave)";
					break;
				case SelectedOutput::AUDIO_B:
					output_name = "AUDIO_B (1Hz triangle wave)";
					break;
				case SelectedOutput::PULSE:
					output_name = "PULSE (1Hz square wave)";
					break;
				default:
					output_name = "UNKNOWN";
			}
			printf("\nOutput selected: %s\n", output_name);
		}
	}
}

Outputs::SelectedOutput Outputs::get_selected_output() const {
	return selected_output_;
}

Outputs::SelectedOutput Outputs::map_pot_to_output_selection(uint16_t pot_value) {
	// Map 0-127 to output selections (same as input mapping)
	// 0 = NONE
	// 1-42 (0-33%) = AUDIO_A
	// 43-84 (33-66%) = AUDIO_B
	// 85-127 (66-100%) = PULSE

	if (pot_value == 0) {
		return SelectedOutput::NONE;
	} else if (pot_value <= 42) {
		return SelectedOutput::AUDIO_A;
	} else if (pot_value <= 84) {
		return SelectedOutput::AUDIO_B;
	} else {
		return SelectedOutput::PULSE;
	}
}

void Outputs::set_ac_coupling(bool use_ac_coupling) {
	if (use_ac_coupling != ac_coupled_) {
		ac_coupled_ = use_ac_coupling;

		brain::io::AudioCvOutCoupling coupling = use_ac_coupling
			? brain::io::AudioCvOutCoupling::kAcCoupled
			: brain::io::AudioCvOutCoupling::kDcCoupled;

		// Set coupling for both channels
		audio_cv_out_.set_coupling(brain::io::AudioCvOutChannel::kChannelA, coupling);
		audio_cv_out_.set_coupling(brain::io::AudioCvOutChannel::kChannelB, coupling);

		printf("Coupling mode: %s\n", use_ac_coupling ? "AC" : "DC");
	}
}

bool Outputs::is_ac_coupled() const {
	return ac_coupled_;
}

uint8_t Outputs::get_selection_indicator_leds() {
	switch (selected_output_) {
		case SelectedOutput::NONE:
			return 0;
		case SelectedOutput::AUDIO_A:
			return 2;
		case SelectedOutput::AUDIO_B:
			return 4;
		case SelectedOutput::PULSE:
			return 6;
		default:
			return 0;
	}
}

void Outputs::generate_triangle_wave(brain::io::AudioCvOutChannel channel) {
	// Calculate current phase in the waveform (0-1000ms for 1Hz)
	absolute_time_t current_time = get_absolute_time();
	uint64_t elapsed_us = absolute_time_diff_us(waveform_start_time_, current_time);
	uint32_t phase_ms = (elapsed_us / 1000) % WAVEFORM_PERIOD_MS;

	// Only update if we've moved to a new millisecond to reduce CPU usage
	if (phase_ms == last_phase_ms_) {
		return;
	}
	last_phase_ms_ = phase_ms;

	// Generate triangle wave: ramp up for first half, ramp down for second half
	// Period = 1000ms, so:
	// 0-500ms: ramp from 0V to 10V
	// 500-1000ms: ramp from 10V to 0V

	float voltage;

	if (phase_ms < 500) {
		// Rising edge: 0V to 10V over 500ms
		// voltage = (phase_ms / 500.0) * 10.0
		// Optimize: voltage = phase_ms * 0.02
		voltage = phase_ms * 0.02f;
	} else {
		// Falling edge: 10V to 0V over 500ms
		// voltage = 10.0 - ((phase_ms - 500) / 500.0) * 10.0
		// Optimize: voltage = 10.0 - (phase_ms - 500) * 0.02
		voltage = 10.0f - (phase_ms - 500) * 0.02f;
	}

	// Clamp to valid range
	if (voltage < MIN_VOLTAGE) voltage = MIN_VOLTAGE;
	if (voltage > MAX_VOLTAGE) voltage = MAX_VOLTAGE;

	// Set DAC output
	audio_cv_out_.set_voltage(channel, voltage);

	// Print periodic voltage updates
	if (phase_ms - last_voltage_print_ms_ >= VOLTAGE_PRINT_INTERVAL_MS) {
		const char* channel_name = (channel == brain::io::AudioCvOutChannel::kChannelA) ? "A" : "B";
		printf("Output %s: %.2fV (phase: %dms/%dms)\r", channel_name, voltage, phase_ms, WAVEFORM_PERIOD_MS);
		last_voltage_print_ms_ = phase_ms;
	}
}

void Outputs::generate_square_wave() {
	// Calculate current phase in the waveform (0-1000ms for 1Hz)
	absolute_time_t current_time = get_absolute_time();
	uint64_t elapsed_us = absolute_time_diff_us(waveform_start_time_, current_time);
	uint32_t phase_ms = (elapsed_us / 1000) % WAVEFORM_PERIOD_MS;

	// Square wave: high for first half (0-500ms), low for second half (500-1000ms)
	bool new_state = (phase_ms < 500);

	// Only update if state changed
	if (new_state != pulse_state_) {
		pulse_state_ = new_state;
		pulse_output_->set(pulse_state_);
		printf("[OUTPUT] Pulse output: %s (phase: %dms/%dms)\n", pulse_state_ ? "HIGH" : "LOW", phase_ms, WAVEFORM_PERIOD_MS);
	}
}

void Outputs::stop_all_outputs() {
	// Set audio outputs to mid-rail (5V) for DC, or 0V for AC
	float stop_voltage = ac_coupled_ ? 0.0f : 5.0f;
	audio_cv_out_.set_voltage(brain::io::AudioCvOutChannel::kChannelA, stop_voltage);
	audio_cv_out_.set_voltage(brain::io::AudioCvOutChannel::kChannelB, stop_voltage);

	// Set pulse output low
	pulse_output_->set(false);
	pulse_state_ = false;
}
