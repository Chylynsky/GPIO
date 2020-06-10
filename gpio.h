#pragma once
#include "bcm2711.h"
#include "gpio_predicates.h"
#include "gpio_events.h"
#include <cstdint>
#include <stdexcept>
#include <functional>

namespace rpi4b
{
	struct __gpio_input_regs
	{
		volatile uint32_t* lev_reg;
		volatile uint32_t* event_reg;
		volatile uint32_t* event_status_reg;
	};

	struct __gpio_output_regs
	{
		volatile uint32_t* set_reg;
		volatile uint32_t* clr_reg;
	};

	template<typename _Dir>
	class gpio : private Select_if<Is_input<_Dir>, __gpio_input_regs, __gpio_output_regs>
	{
		static_assert(Is_input<_Dir> || Is_output<_Dir>, "Template type must be input or output.");

		void select_fsel_reg();

		volatile uint32_t* fsel_reg;

		const uint32_t reg_bit_set_val;	// Value OR'ed with registers responsible for GPIO state (GPSET, GPCLR, ...)
		const uint32_t pin_number;

	public:

		explicit gpio(uint32_t pin_number);
		~gpio();

		gpio() = delete;
		gpio(const gpio&) = delete;
		gpio& operator=(const gpio&) = delete;

		// Output methods

		// Write to GPIO pin
		template<typename _Ty = int>
		Enable_if<Is_output<_Dir>, void> write(_Ty state) noexcept;

		// Input methods

		// Read current GPIO pin state
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, uint8_t> read() noexcept;

		// Set pull-up, pull-down or no resistor
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, void> set_pull(ResistorSelect pull_sel) noexcept;

		// Get current pull type
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, ResistorSelect> get_pull() noexcept;

		// Set event callback
		template<typename _Ev = void>
		Enable_if<Is_input<_Dir> && Is_event<_Ev>, void> set_callback(std::function<void(void)>) noexcept;
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
	inline gpio<_Dir>::gpio(uint32_t pin_number) : reg_bit_set_val{ 1U << (pin_number % 32U) }, pin_number { pin_number }
	{
		// Select GPIO function select register
		select_fsel_reg();

		uint32_t bit_shift = GPFSEL_PIN_BIT_SIZE * (pin_number % 10U);
		// Reset required bits
		*fsel_reg &= ~(0b111U << bit_shift);

		// Enable only when instantiated as input
		if constexpr (Is_input<_Dir>)
		{
			__gpio_output_regs::lev_reg = get_reg_ptr(GPLEV0 + pin_number / 32U);
		}
		else
		{
			*fsel_reg |= static_cast<uint32_t>(GPIOFunctionSelect::GPIO_PIN_AS_OUTPUT) << bit_shift;

			// 31 pins are described by the first GPSET and GPCLR registers
			if (pin_number < 32U)
			{
				__gpio_output_regs::set_reg = get_reg_ptr(GPSET0);
				__gpio_output_regs::clr_reg = get_reg_ptr(GPCLR0);
			}
			else
			{
				__gpio_output_regs::set_reg = get_reg_ptr(GPSET1);
				__gpio_output_regs::clr_reg = get_reg_ptr(GPCLR1);
			}
		}
	}

	template<typename _Dir>
	inline gpio<_Dir>::~gpio()
	{
		// Enable only when instantiated with output
		if constexpr (Is_output<_Dir>)
		{
			*__gpio_output_regs::clr_reg |= reg_bit_set_val;
		}

		*fsel_reg &= ~(0b111U << (GPFSEL_PIN_BIT_SIZE * (pin_number % 10U)));
	}

	template<typename _Dir>
	template<typename _Ty>
	inline Enable_if<Is_output<_Dir>, void>  gpio<_Dir>::write(_Ty state) noexcept
	{
		static_assert(Is_convertible<_Ty, bool>, "Type must be convertible to bool.");

		if (!state)
		{
			*__gpio_output_regs::clr_reg |= reg_bit_set_val;
		}
		else
		{
			*__gpio_output_regs::set_reg |= reg_bit_set_val;
		}
	}

	template<typename _Dir>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, uint8_t> gpio<_Dir>::read() noexcept
	{
		return Enable_if<Is_input<_Ty>, uint8_t>();
	}

	template<typename _Dir>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, void> gpio<_Dir>::set_pull(ResistorSelect pull_sel) noexcept
	{
		return Enable_if<Is_input<_Ty>, void>();
	}

	template<typename _Dir>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, ResistorSelect> gpio<_Dir>::get_pull() noexcept
	{
		return Enable_if<Is_input<_Ty>, ResistorSelect>();
	}

	template<typename _Dir>
	template<typename _Ev>
	inline Enable_if<Is_input<_Dir>&& Is_event<_Ev>, void> gpio<_Dir>::set_callback(std::function<void(void)>) noexcept
	{
		return Enable_if<Is_input<_Dir>&& Is_event<_Ev>, void>();
	}
}