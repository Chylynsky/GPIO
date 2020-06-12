#pragma once
#include "bcm2711.h"
#include "gpio_predicates.h"
#include "gpio_events.h"
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <iostream>
#include <map>
#include <future>
#include <utility>
#include <thread>

namespace rpi4b
{
	using callback_t = std::function<void(void)>;

	struct __gpio_input_regs
	{
		static std::multimap<uint32_t, callback_t> callback_map;
		static std::thread event_poll_thr;
		static std::mutex event_poll_mtx;

		volatile uint32_t* lev_reg;

		static void poll_events()
		{
			volatile uint32_t* base_event_reg = get_reg_ptr(GPEDS0);

			while (true)
			{
				std::lock_guard<std::mutex> lock(event_poll_mtx);

				if (callback_map.empty())
				{
					break;
				}

				for (const auto& callback_pair : callback_map)
				{
					uint32_t reg_index = callback_pair.first / 32U;
					uint32_t bit_index = callback_pair.first % 32U;

					if (((*(base_event_reg + reg_index) >> bit_index) & 1U) == 1U)
					{
						// Clear bit value
						*(base_event_reg + reg_index) &= ~(1U << bit_index);
						// Call event callback
						callback_pair.second();
					}
				}
			}
		}
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
		Enable_if<Is_input<_Ty>, uint32_t> read() const noexcept;

		// Set pull-up, pull-down or no resistor
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, void> set_pull(pull_selection pull_sel) noexcept;

		// Get current pull type
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, pull_selection> get_pull() noexcept;

		// Set event callback
		template<typename _Ev, typename _Ty = _Dir>
		Enable_if<Is_input<_Ty> && Is_event<_Ev>, void> attach_event_callback(callback_t) noexcept;
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
			*fsel_reg |= static_cast<uint32_t>(function_select::gpio_pin_as_output) << (3U * (pin_number % 10U));

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
			{
				std::lock_guard<std::mutex> lock(__gpio_input_regs::event_poll_mtx);

				// Erase event callback from callback map
				__gpio_input_regs::callback_map.erase(pin_number);
			}

			uint32_t bit_clr_val = ~reg_bit_set_val;
			uint32_t reg_index = pin_number / 32U;

			// Clear event detect bits
			*get_reg_ptr(GPREN0 + reg_index) &= bit_clr_val;
			*get_reg_ptr(GPFEN0 + reg_index) &= bit_clr_val;
			*get_reg_ptr(GPHEN0 + reg_index) &= bit_clr_val;
			*get_reg_ptr(GPLEN0 + reg_index) &= bit_clr_val;
			*get_reg_ptr(GPAREN0 + reg_index) &= bit_clr_val;
			*get_reg_ptr(GPAFEN0 + reg_index) &= bit_clr_val;

			// Set pull-down resistor
			set_pull(pull_selection::pull_down);

			// Wait for the polling thread to exit if callback map is empty
			if (!__gpio_input_regs::callback_map.empty())
			{
				return;
			}

			// If the last event-listening gpio pin is being removed, join the polling thread
			if (__gpio_input_regs::event_poll_thr.joinable())
			{
				__gpio_input_regs::event_poll_thr.join();
			}
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
		static_assert(Is_convertible<_Arg, bool>, "Type must be convertible to bool.");

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
	inline Enable_if<Is_input<_Ty>, uint32_t> gpio<_Dir>::read() const noexcept
	{
		return (*__gpio_input_regs::lev_reg >> (pin_number % 32U)) & 0x1U;
	}

	template<typename _Dir>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, void> gpio<_Dir>::set_pull(pull_selection pull_sel) noexcept
	{
		// 16 pins are controlled by each register
		volatile uint32_t* reg_sel = get_reg_ptr(GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));
		*reg_sel &= ~(0b11U << (2U * (pin_number % 16U)));
		*reg_sel |= static_cast<uint32_t>(pull_sel) << (2U * (pin_number % 16U));
	}

	template<typename _Dir>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, pull_selection> gpio<_Dir>::get_pull() noexcept
	{
		// 16 pins are controlled by each register
		volatile uint32_t* reg_sel = get_reg_ptr(GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));
		return static_cast<pull_selection>((*reg_sel >> ((2U * (pin_number % 16U))) & 0x3U));
	}

	template<typename _Dir>
	template<typename _Ev, typename _Ty>
	inline Enable_if<Is_input<_Ty> && Is_event<_Ev>, void> gpio<_Dir>::attach_event_callback(callback_t callback) noexcept
	{
		// Get event register based on event type
		volatile uint32_t* event_reg = get_reg_ptr(Event_reg_offs<_Ev> + pin_number / 32U);
		// Set bit responsible for the selected pin
		*event_reg |= reg_bit_set_val;

		if (__gpio_input_regs::callback_map.empty())
		{
			if (__gpio_input_regs::event_poll_thr.joinable())
			{
				__gpio_input_regs::event_poll_thr.join();
			}

			std::lock_guard<std::mutex> lock(__gpio_input_regs::event_poll_mtx);
			__gpio_input_regs::callback_map.insert(std::make_pair(pin_number, callback));
			__gpio_input_regs::event_poll_thr = std::thread(__gpio_input_regs::poll_events);
		}
		else
		{
			std::lock_guard<std::mutex> lock(__gpio_input_regs::event_poll_mtx);
			__gpio_input_regs::callback_map.insert(std::make_pair(pin_number, callback));
		}
	}
}