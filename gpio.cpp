#include "gpio.h"

namespace rpi4b
{
	gpio::gpio(uint32_t pin_number, GPIODirection direction) : gpfsel_reg{ nullptr }, pin_number { pin_number }, direction{ direction }
	{
		if (direction == GPIODirection::OUTPUT) {
			auto fsel = GPIOFunctionSelect::GPIO_PIN_AS_OUTPUT;

			// Set GPIO function select register value for the selected pin
			if (pin_number <= 9) {
				gpfsel_reg = reinterpret_cast<uint32_t*>(GPFSEL0);
				bit_offset = pin_number * GPFSEL_PIN_BIT_SIZE;
			}
			else if (pin_number >= 10 && pin_number <= 19) {
				gpfsel_reg = reinterpret_cast<uint32_t*>(GPFSEL1);
				bit_offset = (pin_number - 10) * GPFSEL_PIN_BIT_SIZE;
			}
			else if (pin_number >= 20 && pin_number <= 27) {
				gpfsel_reg = reinterpret_cast<uint32_t*>(GPFSEL2);
				bit_offset = (pin_number - 20) * GPFSEL_PIN_BIT_SIZE;
			}
			else {
				throw std::runtime_error("GPIO pins available are 0 to 27.");
			}

			*gpfsel_reg |= static_cast<uint32_t>(fsel) << bit_offset;
		}
	}

	gpio::~gpio()
	{
		if (direction == GPIODirection::OUTPUT) {
			*gpfsel_reg |= static_cast<uint32_t>(GPIOFunctionSelect::GPIO_PIN_AS_INPUT) << bit_offset;
		}
	}
}