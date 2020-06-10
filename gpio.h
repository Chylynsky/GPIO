#pragma once
#include "bcm2711.h"
#include "gpio_predicates.h"
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace rpi4b
{
	enum class GPIOState
	{
		LOW,
		HIGH
	};

	template<typename _Dir>
	class gpio
	{
		static_assert(Is_input<_Dir>() || Is_output<_Dir>(), "Template type must be input or output.");

		void select_fsel_reg();

		volatile uint32_t* fsel_reg;
		volatile uint32_t* set_reg;
		volatile uint32_t* clr_reg;

		uint32_t pin_number;

	public:

		explicit gpio(uint32_t pin_number);

		~gpio();

		gpio() = delete;
		gpio(const gpio&) = delete;
		gpio& operator=(const gpio&) = delete;

		std::enable_if_t<Is_output<_Dir>(), void> write(uint32_t state) noexcept;
	};

	template<typename _Dir>
	inline void gpio<_Dir>::select_fsel_reg()
	{
		switch (pin_number / 10U)
		{
		case 0: fsel_reg = get_reg_ptr(GPFSEL0); break;
		case 1: fsel_reg = get_reg_ptr(GPFSEL1); break;
		case 2: fsel_reg = get_reg_ptr(GPFSEL2); break;
		case 3: fsel_reg = get_reg_ptr(GPFSEL3); break;
		case 4: fsel_reg = get_reg_ptr(GPFSEL4); break;
		case 5: fsel_reg = get_reg_ptr(GPFSEL5); break;
		default: throw std::runtime_error("Pin number out of range."); break;
		}
	}

	template<typename _Dir>
	inline gpio<_Dir>::gpio(uint32_t pin_number) : pin_number{ pin_number }
	{
		// Select GPIO function select register
		select_fsel_reg();
		*fsel_reg &= ~(0b111U << (GPFSEL_PIN_BIT_SIZE * (pin_number % 10U)));

		if constexpr (Is_output<_Dir>)
		{
			*fsel_reg |= static_cast<uint32_t>(GPIOFunctionSelect::GPIO_PIN_AS_OUTPUT) << (GPFSEL_PIN_BIT_SIZE * (pin_number % 10U));

			// 31 pins are described by the first GPSET and GPCLR registers
			if (pin_number < 32U)
			{
				set_reg = get_reg_ptr(GPSET0);
				clr_reg = get_reg_ptr(GPCLR0);
			}
			else
			{
				set_reg = get_reg_ptr(GPSET1);
				clr_reg = get_reg_ptr(GPCLR1);
			}
		}
	}

	template<typename _Dir>
	inline gpio<_Dir>::~gpio()
	{
		*clr_reg |= 1U << pin_number;
		*fsel_reg &= ~(0b111U << (GPFSEL_PIN_BIT_SIZE * (pin_number % 10U)));
	}

	template<typename _Dir>
	inline std::enable_if_t<Is_output<_Dir>(), void>  gpio<_Dir>::write(uint32_t state) noexcept
	{
		if (!state)
		{
			*clr_reg |= 1U << (pin_number % 32U);
		}
		else
		{
			*set_reg |= 1U << (pin_number % 32U);
		}
	}
}