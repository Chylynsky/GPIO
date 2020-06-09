#include "gpio.h"

namespace rpi4b
{
	GPIO::GPIO(uint32_t pinNumber, GPIODirection direction) : pinNumber { pinNumber }, direction{ direction }
	{
		// Select GPIO function select register
		switch (pinNumber / 10U)
		{
		case 0: functionSelectRegister = GetRegisterPtr(GPFSEL0); break;
		case 1: functionSelectRegister = GetRegisterPtr(GPFSEL1); break;
		case 2: functionSelectRegister = GetRegisterPtr(GPFSEL2); break;
		case 3: functionSelectRegister = GetRegisterPtr(GPFSEL3); break;
		case 4: functionSelectRegister = GetRegisterPtr(GPFSEL4); break;
		case 5: functionSelectRegister = GetRegisterPtr(GPFSEL5); break;
		default: throw std::runtime_error("Pin number out of range."); break;
		}

		// 31 pins are described by the first GPSET and GPCLR registers
		if (pinNumber < 32U) 
		{
			setRegister = GetRegisterPtr(GPSET0);
			clrRegister = GetRegisterPtr(GPCLR0);
		}
		else 
		{
			setRegister = GetRegisterPtr(GPSET1);
			clrRegister = GetRegisterPtr(GPCLR1);
		}

		// If the direction is set to INPUT there is no need to change the register value
		if (direction == GPIODirection::INPUT) 
		{
			return;
		}

		auto functionSelected = GPIOFunctionSelect::GPIO_PIN_AS_OUTPUT;
		*functionSelectRegister &= ~(0b111U << (GPFSEL_PIN_BIT_SIZE * (pinNumber % 10U)));
		*functionSelectRegister |= static_cast<uint32_t>(functionSelected) << (GPFSEL_PIN_BIT_SIZE * (pinNumber % 10U));
	}

	GPIO::~GPIO()
	{
		if (direction == GPIODirection::OUTPUT) 
		{
			*clrRegister |= 1U << pinNumber;
			*functionSelectRegister &= ~(0b111U << (GPFSEL_PIN_BIT_SIZE * (pinNumber % 10U)));
		}
	}

	void GPIO::Write(uint32_t state) noexcept
	{
		if (!state) 
		{
			*clrRegister |= 1U << (pinNumber % 32U);
		}
		else 
		{
			*setRegister |= 1U << (pinNumber % 32U);
		}
	}

	void GPIO::Write(bool state) noexcept
	{
		if (!state)
		{
			*clrRegister |= 1U << (pinNumber % 32U);
		}
		else
		{
			*setRegister |= 1U << (pinNumber % 32U);
		}
	}

	void GPIO::Write(GPIOState state) noexcept
	{
		if (state == GPIOState::LOW)
		{
			*clrRegister |= 1U << (pinNumber % 32U);
		}
		else
		{
			*setRegister |= 1U << (pinNumber % 32U);
		}
	}
}