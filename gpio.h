#pragma once
#include "bcm2711.h"
#include "gpio_predicates.h"
#include "gpio_events.h"
#include "dispatch_queue.h"
#include <poll.h>
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <iostream>
#include <map>
#include <future>
#include <utility>
#include <thread>

/*
	TO DO:
	- implement linux based polling for /dev/gpiomem instead of while-based
	- run callbacks asynchronously (std::vector<std::future<void>>?)
	- check if deadlocks occurs
*/

namespace rpi4b
{
	using callback_t = std::function<void(void)>;

	template<typename _Reg>
	struct __gpio_input
	{
		static std::multimap<_Reg, callback_t> callback_map;
		static std::thread event_poll_thread;
		static std::mutex event_poll_mtx;

		static dispatch_queue<callback_t> callback_queue;

		volatile _Reg* lev_reg;

		static void poll_events()
		{
			volatile _Reg* base_event_reg = get_reg_ptr(GPEDS0);

			while (true)
			{
				if (callback_map.empty())
				{
					break;
				}

				// poll file descriptor ?

				std::lock_guard<std::mutex> lock(event_poll_mtx);
			}
		}
	};

	template<typename _Reg>
	std::multimap<_Reg, callback_t> __gpio_input<_Reg>::callback_map;
	template<typename _Reg>
	std::mutex __gpio_input<_Reg>::event_poll_mtx;

	template<typename _Reg>
	struct __gpio_output
	{
		volatile _Reg* set_reg;
		volatile _Reg* clr_reg;
	};

	template<typename _Dir, typename _Reg = uint32_t>
	class gpio : private Select_if<Is_input<_Dir>, __gpio_input<_Reg>, __gpio_output<_Reg>>
	{
		static_assert(Is_input<_Dir> || Is_output<_Dir>, "Template type _Dir must be either input or output.");
		static_assert(Is_integral<_Reg>, "Template type _Reg must be integral.");

		static constexpr uint32_t reg_size = 8U * sizeof(_Reg);

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
		Enable_if<Is_input<_Ty>, void> set_pull(pull_selection pull_sel) noexcept;

		// Get current pull type
		template<typename _Ty = _Dir>
		Enable_if<Is_input<_Ty>, pull_selection> get_pull() noexcept;

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
	inline gpio<_Dir, _Reg>::gpio(_Reg pin_number) : reg_bit_set_val { 1U << (pin_number % reg_size) }, pin_number{ pin_number }
	{
		// Select GPIO function select register
		volatile uint32_t* fsel_reg = get_fsel_reg();

		// Clear function select register bits
		*fsel_reg &= ~(0b111U << (3U * (pin_number % 10U)));

		uint32_t reg_index = pin_number / reg_size;

		// Enable only when instantiated with input template parameter
		if constexpr (Is_input<_Dir>)
		{
			__gpio_input<_Reg>::lev_reg = get_reg_ptr(GPLEV0 + reg_index);
		}
		
		// Enable only when instantiated with output template parameter
		if constexpr (Is_output<_Dir>)
		{
			*fsel_reg |= static_cast<uint32_t>(function_select::gpio_pin_as_output) << (3U * (pin_number % 10U));

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
			_Reg bit_clr_val = ~reg_bit_set_val;
			uint32_t reg_index = pin_number / reg_size;

			// Clear event detect bits
			*get_reg_ptr(GPREN0 + reg_index) &= bit_clr_val;
			*get_reg_ptr(GPFEN0 + reg_index) &= bit_clr_val;
			*get_reg_ptr(GPHEN0 + reg_index) &= bit_clr_val;
			*get_reg_ptr(GPLEN0 + reg_index) &= bit_clr_val;
			*get_reg_ptr(GPAREN0 + reg_index) &= bit_clr_val;
			*get_reg_ptr(GPAFEN0 + reg_index) &= bit_clr_val;

			// Set pull-down resistor
			set_pull(pull_selection::pull_down);

			std::lock_guard<std::mutex> lock(__gpio_input<_Reg>::event_poll_mtx);

			// Erase event callback from callback map
			__gpio_input<_Reg>::callback_map.erase(pin_number);
		}

		// Enable only when instantiated with output template parameter
		if constexpr (Is_output<_Dir>)
		{
			*__gpio_output<_Reg>::clr_reg |= reg_bit_set_val;
		}

		*get_fsel_reg() &= ~(0b111U << (3 * (pin_number % 10U)));
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
		return (*__gpio_input<_Reg>::lev_reg >> (pin_number % reg_size)) & 0x1U;
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, void> gpio<_Dir, _Reg>::set_pull(pull_selection pull_sel) noexcept
	{
		// 16 pins are controlled by each register
		volatile _Reg* reg_sel = get_reg_ptr(GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));
		*reg_sel &= ~(0b11U << (2U * (pin_number % 16U)));
		*reg_sel |= static_cast<uint32_t>(pull_sel) << (2U * (pin_number % 16U));
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ty>
	inline Enable_if<Is_input<_Ty>, pull_selection> gpio<_Dir, _Reg>::get_pull() noexcept
	{
		// 16 pins are controlled by each register
		volatile _Reg* reg_sel = get_reg_ptr(GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));
		return static_cast<pull_selection>((*reg_sel >> ((2U * (pin_number % 16U))) & 0x3U));
	}

	template<typename _Dir, typename _Reg>
	template<typename _Ev, typename _Ty>
	inline Enable_if<Is_input<_Ty> && Is_event<_Ev>, void> gpio<_Dir, _Reg>::attach_event_callback(callback_t callback) noexcept
	{
		// Get event register based on event type
		volatile _Reg* event_reg = get_reg_ptr(Event_reg_offs<_Ev> + pin_number / reg_size);
		// Set bit responsible for the selected pin
		*event_reg |= reg_bit_set_val;

		std::lock_guard<std::mutex> lock(__gpio_input<_Reg>::event_poll_mtx);

		if (__gpio_input<_Reg>::callback_map.empty())
		{
			__gpio_input<_Reg>::callback_map.insert(std::make_pair(pin_number, callback));
			__gpio_input<_Reg>::event_poll_fut = std::async(std::launch::async, __gpio_input<_Reg>::poll_events);
		}
		else
		{
			__gpio_input<_Reg>::callback_map.insert(std::make_pair(pin_number, callback));
		}
	}
}