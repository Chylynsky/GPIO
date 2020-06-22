#include <thread>
#include <chrono>
#include <iostream>

#define BCM2711
#include "gpio_direction.h"
#include "gpio.h"

using namespace rpi;
using namespace std;
using namespace std::chrono_literals;
using namespace std::this_thread;

constexpr uint32_t LED_PIN_NUMBER{ 26 };
constexpr uint32_t BTN_PIN_NUMBER{ 25 };

int main()
{
	gpio<dir::output> pinLED(LED_PIN_NUMBER);	// GPIO pin with LED attached
	gpio<dir::input> pinBtn(BTN_PIN_NUMBER);	// GPIO pin with button attached
	
	pinBtn.set_pull(pull_type::pull_up);		// Set pull up to HIGH

	// Create callback function that blinks twice
	auto blink = [&pinLED]
	{
		cout << "Blink!" << endl;
		pinLED.write(1);
		sleep_for(200ms);
		pinLED.write(0);
		sleep_for(200ms);
		pinLED.write(1);
		sleep_for(200ms);
		pinLED.write(0);
	};

	// Call "blink" function when signal is pulled down by the button
	pinBtn.attach_event_callback<irq::falling_edge>(blink);

	cout << "Push the button attached to pin 25 and enjoy the blinking LED!" << endl;

	// Exit ater 60 seconds
	sleep_for(60s);
    return 0;
}