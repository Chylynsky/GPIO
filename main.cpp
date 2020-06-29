#include <thread>
#include <chrono>
#include <iostream>

#define BCM2711
#include "gpio.h"

using namespace rpi;
using namespace std;
using namespace std::chrono_literals;
using namespace std::this_thread;

int main()
{
	constexpr uint32_t LED_PIN_NUMBER{ 26 };
	constexpr uint32_t BTN_PIN_NUMBER{ 25 };

	gpio<dir::output> pinLED(LED_PIN_NUMBER);	// GPIO pin with LED attached
	gpio<dir::input> pinBtn(BTN_PIN_NUMBER);	// GPIO pin with button attached
	
	pinBtn.set_pull(pull::up);	// Set pull up resistor
	// Create callback lambda that makes an LED blink twice
	auto blink = [&pinLED]
	{
		// First blink uses assignment operator
		pinLED = 1;
		sleep_for(100ms);
		pinLED = 0;
		sleep_for(100ms);

		// Second blink uses write function
		pinLED.write(1);
		sleep_for(100ms);
		pinLED.write(0);
		sleep_for(100ms);
	};

	// Call "blink" function when signal is pulled down by the button
	pinBtn.attach_event_callback<irq::falling_edge>(blink);

	cout << "Push the button attached to pin 25 and enjoy the blinking LED!" << endl;

	// Exit ater 60 seconds
	sleep_for(60s);
    return 0;
}