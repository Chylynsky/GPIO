#include <thread>
#include <chrono>
#include <iostream>
#include <bitset>

#define BCM2711
#include "gpio.h"

using namespace rpi;
using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::this_thread;

int main()
{
	/*
	constexpr auto PROGRAM_WAIT_TIME{ 60s };
	constexpr uint32_t LED_PIN_NUMBER{ 26U };
	constexpr uint32_t BTN_PIN_NUMBER{ 25U };

	gpio<dir::output> pinLED(LED_PIN_NUMBER);	// GPIO pin with LED attached
	gpio<dir::input> pinBtn(BTN_PIN_NUMBER);	// GPIO pin with button attached

	pinBtn.set_pull(pull::up);	// Set pull up resistor

	// Create callback lambda that makes an LED blink twice
	auto blink = [&pinLED]()
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

		cout << "Blink!" << endl;
	};

	// Call "blink" lambda when signal is pulled down by the button
	pinBtn.attach_irq_callback<irq::falling_edge>(blink);

	cout << "Push the button attached to pin " << std::to_string(BTN_PIN_NUMBER) << " and enjoy the blinking LED!" << endl;
	cout << "The program will exit after " << PROGRAM_WAIT_TIME.count() << " seconds." << endl;

	sleep_for(PROGRAM_WAIT_TIME);
	*/

	auto callback = []() { cout << "Interrupt detected." << endl; };

	gpio<dir::input> b1{ 11 };
	gpio<dir::input> b2{ 12 };
	gpio<dir::input> b3{ 13 };
	gpio<dir::input> b4{ 14 };
	gpio<dir::input> b5{ 15 };
	gpio<dir::input> b6{ 16 };
	gpio<dir::input> b7{ 17 };
	gpio<dir::input> b8{ 18 };
	gpio<dir::input> b9{ 19 };

	std::vector<gpio<dir::input>*> buttons;
	buttons.push_back(&b1);
	buttons.push_back(&b2);
	buttons.push_back(&b3);
	buttons.push_back(&b4);
	buttons.push_back(&b5);
	buttons.push_back(&b6);
	buttons.push_back(&b7);
	buttons.push_back(&b8);
	buttons.push_back(&b9);

	try
	{
		for (auto& button : buttons)
		{
			button->set_pull(pull::up);
			button->attach_irq_callback<irq::falling_edge>(callback);
		}
	}
	catch (const std::runtime_error& err)
	{
		std::cout << err.what() << endl;
	}

	return 0;
}