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

		volatile uint32_t* get_fsel_reg();

		const uint32_t reg_bit_set_val;	// Value OR'ed with registers responsible for GPIO state (GPSET, GPCLR, ...)
		const uint32_t pin_number;

	public:

		explicit gpio(uint32_t pin_number);
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
		Enable_if<Is_input<_Ty>, uint32_t> read() noexcept;

		// Set pull-up, pull-down or no resistor
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, void> set_pull(gpio_pup_pdn_cntrl_value pull_sel) noexcept;

		// Get current pull type
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, gpio_pup_pdn_cntrl_value> get_pull() noexcept;

		// Set event callback
		template<typename _Ev, typename _Ty = _Dir>
		Enable_if<Is_input<_Ty> && Is_event<_Ev>, void> attach_event_callback(std::function<void(void)>) noexcept;
	};

	template<typename _Dir>
	inline volatile uint32_t* gpio<_Dir>::get_fsel_reg()
	{
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

	template<typename _Dir>
	inline gpio<_Dir>::gpio(uint32_t pin_number) : reg_bit_set_val { 1U << (pin_number % 32U) }, pin_number{ pin_number }
	{
		// Select GPIO function select register
		volatile uint32_t* fsel_reg = get_fsel_reg();

		// Clear function select register bits
		*fsel_reg &= ~(0b111U << (3 * (pin_number % 10U)));
		uint32_t reg_index = pin_number / 32U;

		// Enable only when instantiated with input template parameter
		if constexpr (Is_input<_Dir>)
		{
			__gpio_input_regs::lev_reg = get_reg_ptr(GPLEV0 + reg_index);
		}
		
		// Enable only when instantiated with output template parameter
		if constexpr (Is_output<_Dir>)
		{
			*fsel_reg |= static_cast<uint32_t>(gpio_fsel_reg_value::gpio_pin_as_output) << (3U * (pin_number % 10U));

			// 31 pins are described by the first GPSET and GPCLR registers
			__gpio_output_regs::set_reg = get_reg_ptr(GPSET0 + reg_index);
			__gpio_output_regs::clr_reg = get_reg_ptr(GPCLR0 + reg_index);
		}
	}

	template<typename _Dir>
	inline gpio<_Dir>::~gpio()
	{
		// Enable only when instantiated with input template parameter
		if constexpr (Is_input<_Dir>)
		{
			// Set pull-down resistor
			set_pull(gpio_pup_pdn_cntrl_value::pull_down);
		}

		// Enable only when instantiated with output template parameter
		if constexpr (Is_output<_Dir>)
		{
			*__gpio_output_regs::clr_reg |= reg_bit_set_val;
		}

		*get_fsel_reg() &= ~(0b111U << (3 * (pin_number % 10U)));
	}

	template<typename _Dir>
	template<typename _Arg, typename _Ty>
	inline Enable_if<Is_output<_Ty>, void> gpio<_Dir>::write(_Arg state) noexcept
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
	inline Enable_if<Is_input<_Ty>, uint32_t> gpio<_Dir>::read() noexcept
	{
		return (*__gpio_input_regs::lev_reg >> (pin_number % 32U)) & 0x1U;
	}

	template<typename _Dir>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, void> gpio<_Dir>::set_pull(gpio_pup_pdn_cntrl_value pull_sel) noexcept
	{
		// 16 pins are controlled by each register
		volatile uint32_t* reg_sel = get_reg_ptr(GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));
		*reg_sel &= ~(0b11U << (2U * (pin_number % 16U)));
		*reg_sel |= static_cast<uint32_t>(pull_sel) << (2U * (pin_number % 16U));
	}

	template<typename _Dir>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, gpio_pup_pdn_cntrl_value> gpio<_Dir>::get_pull() noexcept
	{
		// 16 pins are controlled by each register
		volatile uint32_t* reg_sel = get_reg_ptr(GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));
		return static_cast<gpio_pup_pdn_cntrl_value>((*reg_sel >> ((2U * (pin_number % 16U))) & 0x3U));
	}

	template<typename _Dir>
	template<typename _Ev, typename _Ty>
	inline Enable_if<Is_input<_Ty> && Is_event<_Ev>, void> gpio<_Dir>::attach_event_callback(std::function<void(void)>) noexcept
	{
		// Get event register based on event type
		volatile uint32_t* event_reg = get_reg_ptr(Event_reg_offs<_Ev> + pin_number / 32U);
		// Set bit responsible for the selected pin
		//*event_reg |= reg_bit_set_val;
	}
}