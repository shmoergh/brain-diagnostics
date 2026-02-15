#include <pico/stdlib.h>
#include <stdio.h>
#include "hardware/gpio.h"

// Minimal test to check if GPIO4 and GPIO8 are truly independent
int main() {
	stdio_init_all();
	sleep_ms(2000); // Wait for USB serial

	printf("\n=== GPIO Independence Test ===\n");
	printf("Testing if GPIO4 (input) is affected by GPIO8 (output)\n\n");

	// Initialize GPIO4 as INPUT with pull-up (pulse input)
	printf("1. Initializing GPIO4 as INPUT with pull-up...\n");
	gpio_init(4);
	gpio_set_dir(4, GPIO_IN);
	gpio_pull_up(4);

	// Read initial state
	bool gpio4_initial = gpio_get(4);
	printf("   GPIO4 initial state (raw): %d\n", gpio4_initial);
	sleep_ms(100);

	// Initialize GPIO8 as OUTPUT (pulse output)
	printf("\n2. Initializing GPIO8 as OUTPUT...\n");
	gpio_init(8);
	gpio_put(8, true);  // Start HIGH
	gpio_set_dir(8, GPIO_OUT);
	printf("   GPIO8 set to HIGH\n");
	sleep_ms(100);

	// Read GPIO4 again
	bool gpio4_after_init = gpio_get(4);
	printf("   GPIO4 after GPIO8 init (raw): %d\n", gpio4_after_init);

	if (gpio4_initial != gpio4_after_init) {
		printf("   WARNING: GPIO4 changed when GPIO8 was initialized!\n");
	}

	printf("\n3. Toggling GPIO8 continuously...\n");
	printf("   Use oscilloscope to measure:\n");
	printf("   - GPIO8 (pin 11): Should be clean 1Hz square wave\n");
	printf("   - GPIO4 (pin 6): Should stay constant (pulled high)\n");
	printf("   If GPIO4 follows GPIO8, there's hardware coupling!\n\n");
	printf("   Printing status every 10 cycles...\n\n");

	// Toggle GPIO8 continuously at 1Hz for oscilloscope measurement
	int cycle = 0;
	while (true) {
		// Set GPIO8 HIGH
		gpio_put(8, true);
		sleep_ms(500);  // 500ms high
		bool gpio4_when_high = gpio_get(4);

		// Set GPIO8 LOW
		gpio_put(8, false);
		sleep_ms(500);  // 500ms low = 1Hz total
		bool gpio4_when_low = gpio_get(4);

		cycle++;

		printf("PULSE IN STATE: %d", gpio_get(4));

		// Print status every 10 cycles
		if (cycle % 10 == 0) {
			printf("   Cycle %d: GPIO8=HIGH -> GPIO4=%d | GPIO8=LOW -> GPIO4=%d",
			       cycle, gpio4_when_high, gpio4_when_low);

			if (gpio4_when_high != gpio4_when_low) {
				printf(" <- COUPLED!");
			}
			printf("\n");
		}
	}

	return 0;
}
