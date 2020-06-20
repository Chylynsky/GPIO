#pragma once
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <cassert>
#include <iostream>

#include "bcm2711.h"
#include "gpio_predicates.h"
#include "gpio_events.h"
#include "gpio_input.h"
#include "gpio_output.h"
#include "gpio_aliases.h"
#include "gpio_helper.h"

/*
	Main library class gpio is a template allowing access to gpio with direction
	specified by the template type _Dir. Type _Reg is uint32_t by default representing
	32 bit registers.
*/

namespace rpi4b
{
	template<typename _Dir, typename _Reg = reg_t>
	class gpio : private Select_if<Is_input<_Dir>, __gpio_input<_Reg>, __gpio_output<_Reg>>
	{
		static_assert(Is_input<_Dir> || Is_output<_Dir>, "Template type _Dir must be either input or output.");
		static_assert(Is_integral<_Reg>, "Template type _Reg must be integral.");

		volatile _Reg* get_fsel_reg();

		const _Reg reg_bit_set_val;	// Value OR'ed with registers responsible for GPIO state (GPSET, GPCLR, ...)
		const uint32_t pin_number;

	public:

		explicit gpio(_Reg pin_number);
		~gpio();

		gpio() = delete;
		gpio(const gpio&) = delete;
		gpio& operator=(const gpio&) = delete;

		// OUTPUT METHODS

		// Write to GPIO pin
		template<typename _Arg = int, typename _Ty = _Dir>
		Enable_if<Is_output<_Ty>, void> write(_Arg state) noexcept;

		// INPUT METHODS

		// Read current GPIO pin state
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, uint32_t> read() const noexcept;

		// Set pull-up, pull-down or no resistor
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, void> set_pull(pull_type pull_sel) noexcept;

		// Get current pull type
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, pull_type> get_pull() noexcept;

		// Set event callback
		template<typename _Ev, typename _Ty = _Dir>
		Enable_if<Is_input<_Ty> && Is_event<_Ev>, void> attach_event_callback(callback_t) noexcept;
	};

	template<typename _Dir, typename _Reg>
	inline volatile _Reg* gpio<_Dir, _Reg>::get_fsel_reg()
	{
		// Each pin function is described by 3 bits
		switch (pin_number / 10U)
		{
		case 0: return get_reg_ptr(GPFSEL0); break;
		case 1: return get_reg_ptr(GPFSEL1); break;
		case 2: return get_reg_ptr(GPFSEL2); break;
		case 3: return get_reg_ptr(GPFSEL3); break;
		case 4: return get_reg_ptr(GPFSEL4); break;
		case 5: return get_reg_ptr(GPFSEL5); break;
		default: throw std::runtime_error("Pin number out of range."); break;
		}
	}

	template<typename _Dir, typename _Reg>
	inline gpio<_Dir, _Reg>::gpio(_Reg pin_number) : reg_bit_set_val{ 1U << (pin_number % reg_size<_Reg>) }, pin_number{ pin_number }
	{
		// Select GPIO function select register
		volatile uint32_t* fsel_reg = get_fsel_reg();

		// Each pin represented by three bits
		_Reg fsel_bit_shift = (3U * (pin_number % 10U));

		// Clear function select register bits
		*fsel_reg &= ~(0b111U << fsel_bit_shift);

		uint32_t reg_index = pin_number / reg_size<_Reg>;

		// Enable only when instantiated with input template parameter
		if constexpr (Is_input<_Dir>)
		{
			__gpio_input<_Reg>::lev_reg = get_reg_ptr(GPLEV0 + reg_index);
		}
		
		// Enable only when instantiated with output template parameter
		if constexpr (Is_output<_Dir>)
		{
			*fsel_reg |= static_cast<uint32_t>(fsel::gpio_pin_as_output) << fsel_bit_shift;

			// 31 pins are described by the first GPSET and GPCLR registers
			__gpio_output<_Reg>::set_reg = get_reg_ptr(GPSET0 + reg_index);
			__gpio_output<_Reg>::clr_reg = get_reg_ptr(GPCLR0 + reg_index);
		}
	}

	template<typename _Dir, typename _Reg>
	inline gpio<_Dir, _Reg>::~gpio()
	{
		// Enable only when instantiated with input template parameter
		if constexpr (Is_input<_Dir>)
		{
			_Reg reg_bit_clr_val = ~reg_bit_set_val;
			uint32_t reg_index = pin_number / reg_size<_Reg>;

			// Clear event detect bits
			*get_reg_ptr(GPREN0 + reg_index)	&= reg_bit_clr_val;
			*get_reg_ptr(GPFEN0 + reg_index)	&= reg_bit_clr_val;
			*get_reg_ptr(GPHEN0 + reg_index)	&= reg_bit_clr_val;
			*get_reg_ptr(GPLEN0 + reg_index)	&= reg_bit_clr_val;
			*get_reg_ptr(GPAREN0 + reg_index)	&= reg_bit_clr_val;
			*get_reg_ptr(GPAFEN0 + reg_index)	&= reg_bit_clr_val;

			// Set pull-down resistor
			set_pull(pull_type::pull_down);

			__gpio_input<_Reg>::callback_map.erase(pin_number);
		}

		// Enable only when instantiated with output template parameter
		if constexpr (Is_output<_Dir>)
		{
			*__gpio_output<_Reg>::clr_reg |= reg_bit_set_val;
		}

		*(get_fsel_reg()) &= ~(0b111U << (3U * (pin_number % 10U)));
	}

	template<typename _Dir, typename _Reg>
	template<typename _Arg, typename _Ty>
	inline Enable_if<Is_output<_Ty>, void> gpio<_Dir, _Reg>::write(_Arg state) noexcept
	{
		static_assert(Is_convertible<_Arg, bool>, "Type must be convertible to bool.");

		if (!state)
		{
			*__gpio_output<_Reg>::clr_reg |= reg_bit_set_val;
		}
		else
		{
			*__gpio_output<_Reg>::set_reg |= reg_bit_set_val;
		}
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, uint32_t> gpio<_Dir, _Reg>::read() const noexcept
	{
		return (*__gpio_input<_Reg>::lev_reg >> (pin_number % reg_size<_Reg>)) & 1U;
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, void> gpio<_Dir, _Reg>::set_pull(pull_type pull_sel) noexcept
	{
		// 16 pins are controlled by each register
		volatile _Reg* reg_sel = get_reg_ptr(GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));

		 // Each pin is represented by two bits, 16 pins described by each register
		_Reg bit_shift = 2U * (pin_number % 16U);

		// Clear the bit and set it
		*reg_sel &= ~(0b11U << bit_shift);
		*reg_sel |= static_cast<_Reg>(pull_sel) << bit_shift;
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, pull_type> gpio<_Dir, _Reg>::get_pull() noexcept
	{
		// 16 pins are controlled by each register
		volatile _Reg* reg_sel = get_reg_ptr(GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));
		return static_cast<pull_type>(*reg_sel >> (2U * (pin_number % 16U)) & 0b11U);
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ev, typename _Ty>
	inline Enable_if<Is_input<_Ty> && Is_event<_Ev>, void> gpio<_Dir, _Reg>::attach_event_callback(callback_t callback) noexcept
	{
		// Get event register based on event type
		volatile _Reg* event_reg = get_reg_ptr(Event_reg_offs<_Ev> + pin_number / reg_size<_Reg>);

		// Clear then set bit responsible for the selected pin
		*event_reg &= ~reg_bit_set_val;
		*event_reg |= reg_bit_set_val;

		// Add pin number - callback function pair to the callback map
		__gpio_input<_Reg>::callback_map.insert(std::make_pair(pin_number, callback));
	}
}