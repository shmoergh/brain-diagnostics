#include "inputs.h"
#include <stdio.h>
#include <brain-common/brain-common.h>

Inputs::Inputs()
	: selected_input_(SelectedInput::NONE),
	  vu_peak_hold_(0) {
	vu_peak_time_ = get_absolute_time();
}

void Inputs::init() {
	printf("Initializing inputs...\n");

	// Initialize audio/CV inputs
	if (!audio_cv_in_.init()) {
		printf("WARNING: Failed to initialize audio/CV inputs\n");
	}

	// Initialize pulse input
	pulse_.begin();

	printf("Inputs initialized\n");
}

void Inputs::update() {
	// Update audio/CV readings (non-blocking)
	audio_cv_in_.update();

	// Poll pulse input for edge detection (non-blocking)
	pulse_.poll();
}

void Inputs::set_selected_input(SelectedInput selected) {
	if (selected != selected_input_) {
		selected_input_ = selected;
		// Reset VU meter state when switching inputs
		vu_peak_hold_ = 0;
		vu_peak_time_ = get_absolute_time();
	}
}

Inputs::SelectedInput Inputs::get_selected_input() const {
	return selected_input_;
}

Inputs::SelectedInput Inputs::map_pot_to_input_selection(uint16_t pot_value) {
	// Map 0-127 to input selections
	// 0 = NONE
	// 1-42 (0-33%) = AUDIO_A
	// 43-84 (33-66%) = AUDIO_B
	// 85-127 (66-100%) = PULSE

	if (pot_value == 0) {
		return SelectedInput::NONE;
	} else if (pot_value <= 42) {
		return SelectedInput::AUDIO_A;
	} else if (pot_value <= 84) {
		return SelectedInput::AUDIO_B;
	} else {
		return SelectedInput::PULSE;
	}
}

uint8_t Inputs::get_vu_meter_level() {
	if (selected_input_ != SelectedInput::AUDIO_A &&
	    selected_input_ != SelectedInput::AUDIO_B) {
		return 0;
	}

	// Get raw ADC value for selected channel
	uint16_t raw_adc;
	if (selected_input_ == SelectedInput::AUDIO_A) {
		raw_adc = audio_cv_in_.get_raw_channel_a();
	} else {
		raw_adc = audio_cv_in_.get_raw_channel_b();
	}

	// Calculate VU level with peak hold
	return calculate_vu_level(raw_adc);
}

bool Inputs::is_pulse_high() {
	return pulse_.read();
}

uint8_t Inputs::get_selection_indicator_leds() {
	switch (selected_input_) {
		case SelectedInput::NONE:
			return 0;
		case SelectedInput::AUDIO_A:
			return 2;
		case SelectedInput::AUDIO_B:
			return 4;
		case SelectedInput::PULSE:
			return 6;
		default:
			return 0;
	}
}

uint8_t Inputs::calculate_vu_level(uint16_t raw_adc) {
	// ADC is 12-bit: 0-4095
	// DC-coupled inputs can have DC offset
	// Center point is around 2048 (mid-rail)
	// Calculate absolute deviation from center

	int32_t center = 2048;
	int32_t deviation = raw_adc - center;
	if (deviation < 0) {
		deviation = -deviation;  // Absolute value
	}

	// Update peak hold
	uint16_t current_level = (uint16_t)deviation;
	absolute_time_t current_time = get_absolute_time();

	if (current_level > vu_peak_hold_) {
		vu_peak_hold_ = current_level;
		vu_peak_time_ = current_time;
	} else {
		// Decay peak hold after hold time
		uint64_t elapsed_ms = absolute_time_diff_us(vu_peak_time_, current_time) / 1000;
		if (elapsed_ms > VU_PEAK_HOLD_MS) {
			// Gradual decay
			if (vu_peak_hold_ > current_level) {
				vu_peak_hold_ = current_level;
			}
		}
	}

	// Map deviation (0-2047 max) to LED count (0-6)
	// Use peak hold value for display
	// Scale: ~340 per LED (2047 / 6)
	uint8_t num_leds;

	if (vu_peak_hold_ < 170) {
		num_leds = 0;
	} else if (vu_peak_hold_ < 510) {
		num_leds = 1;
	} else if (vu_peak_hold_ < 850) {
		num_leds = 2;
	} else if (vu_peak_hold_ < 1190) {
		num_leds = 3;
	} else if (vu_peak_hold_ < 1530) {
		num_leds = 4;
	} else if (vu_peak_hold_ < 1870) {
		num_leds = 5;
	} else {
		num_leds = 6;
	}

	return num_leds;
}
