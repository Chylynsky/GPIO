#pragma once
#include "bcm2711.h"
#include <cassert>
#include <stdexcept>

namespace rpi4b
{
	enum class GPIODirection
	{
		INPUT,
		OUTPUT
	};

	class gpio
	{
		uint32_t* gpfsel_reg;
		uint32_t bit_offset;
		uint32_t pin_number;
		GPIODirection direction;

	public:

		explicit gpio(uint32_t pin_number, GPIODirection direction = GPIODirection::INPUT);
		~gpio();
	};
}