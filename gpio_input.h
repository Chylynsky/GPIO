#pragma once
#include <map>
#include <thread>
#include <mutex>
#include "gpio_aliases.h"
#include "dispatch_queue.h"

namespace rpi4b
{
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
}