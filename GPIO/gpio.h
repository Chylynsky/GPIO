#pragma once
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <iostream>

#include "gpio_direction.h"
#include "gpio_predicates.h"
#include "gpio_events.h"
#include "gpio_input.h"
#include "gpio_output.h"
#include "gpio_aliases.h"
#include "gpio_helper.h"

#if defined(BCM2711)
#include "bcm2711.h"
#else
static_assert(0, "Processor model must be specified as a macro definition before including gpio.h.");
#endif

namespace rpi
{
	/*
		Template class gpio allows access to gpio with direction
		specified by the template type _Dir and register type _Reg.
	*/
	template<typename _Dir, typename _Reg = __addr::reg_t>
	class gpio : private __pred::__Select_if<__pred::__Is_input<_Dir>, __gpio_input<_Reg>, __gpio_output<_Reg>>
	{
		static_assert(__pred::__Is_direction<_Dir>, "Template type _Dir must be either dir::input or dir::output.");
		static_assert(__pred::__Is_integral<_Reg>, "Template type _Reg must be integral.");

		// Select FSEL register offset based on GPIO pin number.
		volatile _Reg* get_fsel_reg();

		const _Reg reg_bit_set_val;	// Value OR'ed with registers responsible for GPIO state (GPSET, GPCLR, ...)
		const uint32_t pin_number;	// GPIO pin number.

	public:

		explicit gpio(uint32_t pin_number);
		~gpio();

		// Output methods

		template<typename _Arg = int, typename _Ty = _Dir>
		__pred::__Enable_if<__pred::__Is_output<_Ty>, void> operator=(_Arg state) noexcept;

		// Write to GPIO pin
		template<typename _Arg = int, typename _Ty = _Dir>
		__pred::__Enable_if<__pred::__Is_output<_Ty>, void> write(_Arg state) noexcept;

		// Input methods.

		// Read current GPIO pin state.
		template<typename _Ty = _Dir>
		__pred::__Enable_if<__pred::__Is_input<_Ty>, uint32_t> read() const noexcept;

		// Set pull-up, pull-down or none.
		template<typename _Ty = _Dir>
		__pred::__Enable_if<__pred::__Is_input<_Ty>, void> set_pull(pull pull_sel) noexcept;

		// Get current pull type.
		template<typename _Ty = _Dir>
		__pred::__Enable_if<__pred::__Is_input<_Ty>, pull> get_pull() noexcept;

		// Set event callback.
		template<typename _Ev, typename _Ty = _Dir>
		__pred::__Enable_if<__pred::__Is_input<_Ty> && __pred::__Is_event<_Ev>, void> attach_irq_callback(callback_t callback);

		// Deleted methods.

		gpio() = delete;
		gpio(const gpio&) = delete;
		gpio(gpio&&) = delete;
		gpio& operator=(const gpio&) = delete;
		gpio& operator=(gpio&&) = delete;
	};

	template<typename _Dir, typename _Reg>
	inline volatile _Reg* gpio<_Dir, _Reg>::get_fsel_reg()
	{
		/* 
			Each pin function is described by 3 bits,
			each register controls 10 pins
		*/
		switch (pin_number / 10U)
		{
		case 0: return __get_reg_ptr<_Reg>(__addr::GPFSEL0); break;
		case 1: return __get_reg_ptr<_Reg>(__addr::GPFSEL1); break;
		case 2: return __get_reg_ptr<_Reg>(__addr::GPFSEL2); break;
		case 3: return __get_reg_ptr<_Reg>(__addr::GPFSEL3); break;
		case 4: return __get_reg_ptr<_Reg>(__addr::GPFSEL4); break;
		case 5: return __get_reg_ptr<_Reg>(__addr::GPFSEL5); break;
		default: throw std::runtime_error("Pin number out of range."); break;
		}
	}

	template<typename _Dir, typename _Reg>
	inline gpio<_Dir, _Reg>::gpio(uint32_t pin_number) : reg_bit_set_val{ 1U << (pin_number % reg_size<_Reg>) }, pin_number{ pin_number }
	{
		// Select GPIO function select register.
		volatile _Reg* fsel_reg = get_fsel_reg();

		// Each pin represented by three bits.
		_Reg fsel_bit_shift = (3U * (pin_number % 10U));

		// Clear function select register bits.
		*fsel_reg &= ~(0b111U << fsel_bit_shift);

		uint32_t reg_index = pin_number / reg_size<_Reg>;

		// Enable only when instantiated with input template parameter.
		if constexpr (__pred::__Is_input<_Dir>)
		{
			*fsel_reg |= static_cast<uint32_t>(__function_select::gpio_pin_as_input) << fsel_bit_shift;
			__gpio_input<_Reg>::lev_reg = __get_reg_ptr<_Reg>(__addr::GPLEV0 + reg_index);
		}
		
		// Enable only when instantiated with output template parameter.
		if constexpr (__pred::__Is_output<_Dir>)
		{
			*fsel_reg |= static_cast<uint32_t>(__function_select::gpio_pin_as_output) << fsel_bit_shift;

			// 31 pins are described by the first GPSET and GPCLR registers.
			__gpio_output<_Reg>::set_reg = __get_reg_ptr<_Reg>(__addr::GPSET0 + reg_index);
			__gpio_output<_Reg>::clr_reg = __get_reg_ptr<_Reg>(__addr::GPCLR0 + reg_index);
		}
	}

	template<typename _Dir, typename _Reg>
	inline gpio<_Dir, _Reg>::~gpio()
	{
		// Enable only when instantiated with input template parameter.
		if constexpr (__pred::__Is_input<_Dir>)
		{
			_Reg reg_bit_clr_val = ~reg_bit_set_val;

			// Clear event detect bits.
			for (volatile _Reg* reg : __gpio_input<_Reg>::event_regs_used)
			{
				*reg &= reg_bit_clr_val;
			}

			// Set pull-down resistor.
			set_pull(pull::down);

			__gpio_input<_Reg>::callback_map.erase(pin_number);
		}

		// Enable only when instantiated with output template parameter.
		if constexpr (__pred::__Is_output<_Dir>)
		{
			*__gpio_output<_Reg>::clr_reg |= reg_bit_set_val;
		}

		// Reset function select register.
		*(get_fsel_reg()) &= ~(0b111U << (3U * (pin_number % 10U)));
	}

	template<typename _Dir, typename _Reg>
	template<typename _Arg, typename _Ty>
	inline __pred::__Enable_if<__pred::__Is_output<_Ty>, void> gpio<_Dir, _Reg>::operator=(_Arg state) noexcept
	{
		static_assert(__pred::__Is_convertible<_Arg, bool>, "Type must be convertible to bool.");

		// Set '1' in SET or CLR register.
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
	template<typename _Arg, typename _Ty>
	inline __pred::__Enable_if<__pred::__Is_output<_Ty>, void> gpio<_Dir, _Reg>::write(_Arg state) noexcept
	{
		*this = state;
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ty>
	inline __pred::__Enable_if<__pred::__Is_input<_Ty>, uint32_t> gpio<_Dir, _Reg>::read() const noexcept
	{
		return (*__gpio_input<_Reg>::lev_reg >> (pin_number % reg_size<_Reg>)) & 1U;
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ty>
	inline __pred::__Enable_if<__pred::__Is_input<_Ty>, void> gpio<_Dir, _Reg>::set_pull(pull pull_sel) noexcept
	{
		// 16 pins are controlled by each register.
		volatile _Reg* reg_sel = __get_reg_ptr<_Reg>(__addr::GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));

		 // Each pin is represented by two bits, 16 pins described by each register.
		_Reg bit_shift = 2U * (pin_number % 16U);

		// Clear the bit and set it.
		*reg_sel &= ~(0b11U << bit_shift);
		*reg_sel |= static_cast<_Reg>(pull_sel) << bit_shift;
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ty>
	inline __pred::__Enable_if<__pred::__Is_input<_Ty>, pull> gpio<_Dir, _Reg>::get_pull() noexcept
	{
		// 16 pins are controlled by each register.
		volatile _Reg* reg_sel = __get_reg_ptr<_Reg>(__addr::GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));
		return static_cast<pull>(*reg_sel >> (2U * (pin_number % 16U)) & 0b11U);
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ev, typename _Ty>
	inline __pred::__Enable_if<__pred::__Is_input<_Ty> && __pred::__Is_event<_Ev>, void> 
		gpio<_Dir, _Reg>::attach_irq_callback(callback_t callback)
	{
		// Get event register based on event type.
		volatile _Reg* event_reg = __get_reg_ptr<_Reg>(__Event_reg_offs<_Reg, _Ev> + pin_number / reg_size<_Reg>);

		// Clear then set bit responsible for the selected pin.
		*event_reg &= ~reg_bit_set_val;
		*event_reg |= reg_bit_set_val;

		__gpio_input<_Reg>::event_regs_used.push_back(event_reg);
		__gpio_input<_Reg>::callback_map.insert(std::make_pair(pin_number, callback));
	}
}