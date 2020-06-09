#pragma once
#include "bcm2711.h"
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace rpi4b
{

	enum class GPIODirection
	{
		INPUT,
		OUTPUT
	};

	enum class GPIOState
	{
		LOW,
		HIGH
	};

	class GPIO
	{
		volatile uint32_t* functionSelectRegister;
		volatile uint32_t* setRegister;
		volatile uint32_t* clrRegister;

		uint32_t pinNumber;
		GPIODirection direction;

	public:

		explicit GPIO(uint32_t pinNumber, GPIODirection direction = GPIODirection::INPUT);
		~GPIO();

		GPIO(const GPIO&) = delete;
		GPIO& operator=(const GPIO&) = delete;

		void Write(uint32_t state) noexcept;
		void Write(bool state) noexcept;
		void Write(GPIOState state) noexcept;
	};
}