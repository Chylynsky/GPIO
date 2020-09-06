#pragma once
#include <map>
#include <thread>
#include <mutex>
#include <functional>
#include <future>
#include <condition_variable>
#include <utility>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <poll.h>

#include "gpio_aliases.h"
#include "gpio_helper.h"
#include "dispatch_queue.h"

#if defined(BCM2711)
#include "bcm2711.h"
#else
static_assert(0, "No processor model was specified.")
#endif

namespace rpi
{
	/*
		Template class holding key - value pairs where key is the GPIO pin number
		and value is the callback function which gets executed when specified event
		occures. Its main task is to control whether an event has occured and if so,
		to pass it into dispatch queue.
	*/
	template<typename _Reg, typename _Fun>
	class __gpio_callback_map
	{
		static_assert(__pred::__Is_integral<_Reg>, "_Reg must be of integral type.");

		static constexpr auto INTERRUPT_THREAD_WAIT_TIME = std::chrono::milliseconds(50);

		std::multimap<_Reg, _Fun> callback_map;		// Multimap where key - pin_number, value - callback function.
		std::future<void> event_poll_thread;				// Thread on which events are polled.
		std::mutex event_poll_mtx;					// Mutex for resource access control.
		std::condition_variable event_poll_cond;	// Puts the thread to sleep when callback_map is empty.
		std::atomic<bool> event_poll_thread_exit;	// Loop control for event_poll_thread.
		__dispatch_queue<_Fun> callback_queue;		// When an event occurs, the corresponding callback function is pushed here.

	public:

		// Constructor.
		__gpio_callback_map();
		// Destructor.
		~__gpio_callback_map();

		// Main event_poll_thread function.
		void poll_events();
		// Insert new key-value pair.
		void insert(std::pair<_Reg, _Fun>&& key_value_pair);
		// Erase all callback functions for the specified pin.
		void erase(_Reg key);
	};

	template<typename _Reg, typename _Fun>
	inline __gpio_callback_map<_Reg, _Fun>::__gpio_callback_map() : event_poll_thread_exit{ false }
	{
		event_poll_thread = std::async(std::launch::async, [this]() { poll_events(); });
	}

	template<typename _Reg, typename _Fun>
	inline __gpio_callback_map<_Reg, _Fun>::~__gpio_callback_map()
	{
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			callback_map.clear();
			event_poll_thread_exit = true;
		} // Release the lock and notify waiting thread.
		event_poll_cond.notify_one();

		event_poll_thread.wait_for(INTERRUPT_THREAD_WAIT_TIME);
	}

	template<typename _Reg, typename _Fun>
	inline void __gpio_callback_map<_Reg, _Fun>::poll_events()
	{
		volatile _Reg* base_event_reg = __get_reg_ptr<_Reg>(__addr::GPEDS0);

		while (!event_poll_thread_exit)
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			if (callback_map.empty())
			{
				// Wait untill callback_map is not empty.
				event_poll_cond.wait(lock, [this]() { return (!callback_map.empty() || event_poll_thread_exit); });
			}
			else
			{
				for (const std::pair<_Reg, _Fun>& entry : callback_map)
				{
					_Reg pin_no = entry.first;
					_Reg reg_sel = pin_no / reg_size<_Reg>;
					_Reg event_occured = (base_event_reg[reg_sel] >> pin_no) & 1U;

					if (event_occured)
					{
						// Clear event register.
						*(base_event_reg + reg_sel) |= (1U << pin_no);
						callback_queue.push(entry.second);
					}
				}
			}
		}
	}

	template<typename _Reg, typename _Fun>
	inline void __gpio_callback_map<_Reg, _Fun>::insert(std::pair<_Reg, _Fun>&& key_val)
	{
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			callback_map.insert(key_val);
		} // Release the lock and notify waiting thread.
		event_poll_cond.notify_one();
	}

	template<typename _Reg, typename _Fun>
	inline void __gpio_callback_map<_Reg, _Fun>::erase(_Reg key)
	{
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			callback_map.erase(key);
		} // Release the lock and notify waiting thread.
		event_poll_cond.notify_one();
	}
}