#include <pico/stdlib.h>
#include <stdio.h>
#include "diagnostics.h"

int main() {
	stdio_init_all();

	// Create and initialize diagnostics system
	Diagnostics diagnostics;
	diagnostics.init();

	// Main loop
	while (true) {
		diagnostics.update();
	}

	return 0;
}
