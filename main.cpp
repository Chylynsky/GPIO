#include <thread>
#include <chrono>
#include <iostream>
#include "gpio.h"

using namespace std::chrono_literals;
using namespace std::this_thread;
using namespace rpi4b;

int main()
{
	gpio<output> pinLED(26);	// GPIO pin with LED attached
	gpio<input> pinBtn(25);		// GPIO pin with button attached

	pinBtn.set_pull(pull_type::pull_up);	// Set pull up to HIGH

	// Create callback function that blinks once
	auto blink = [&pinLED]
	{
		pinLED.write(1);
		sleep_for(500ms);
		pinLED.write(0);
	};

	// Call "blink" function when signal is pulled down be the button
	pinBtn.attach_event_callback<irq::falling_edge>(blink);

	// Exit ater 60 seconds
	sleep_for(60s);
    return 0;
}