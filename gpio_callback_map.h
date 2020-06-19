#pragma once
#include <map>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <utility>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <poll.h>

#include "bcm2711.h"
#include "gpio_aliases.h"
#include "gpio_helper.h"
#include "dispatch_queue.h"

namespace rpi4b
{
	template<typename _Reg, typename _Fun>
	class gpio_callback_map
	{
		std::multimap<_Reg, callback_t> callback_map;
		std::thread event_poll_thread;
		std::mutex event_poll_mtx;
		std::condition_variable event_poll_cond;
		std::atomic<bool> event_poll_thread_exit;
		dispatch_queue<callback_t> callback_queue;

	public:

		gpio_callback_map();
		~gpio_callback_map();

		void poll_events();
		void push(std::pair<_Reg, _Fun>&& key_val);
	};

	template<typename _Reg, typename _Fun>
	inline gpio_callback_map<_Reg, _Fun>::gpio_callback_map() : event_poll_thread_exit{ false }
	{
		event_poll_thread = std::thread(std::bind(&gpio_callback_map<_Reg, _Fun>::poll_events, this));
	}

	template<typename _Reg, typename _Fun>
	inline gpio_callback_map<_Reg, _Fun>::~gpio_callback_map()
	{
		event_poll_thread_exit = true;
		event_poll_cond.notify_one();

		if (event_poll_thread.joinable())
		{
			event_poll_thread.join();
		}
	}

	template<typename _Reg, typename _Fun>
	inline void gpio_callback_map<_Reg, _Fun>::poll_events()
	{
		volatile _Reg* base_event_reg = get_reg_ptr(GPEDS0);

		pollfd mem;
		
		mem.fd = open("/dev/gpiomem", O_RDWR | O_SYNC);
		mem.events = POLLIN;

		if (mem.fd < 0)
		{
			throw std::runtime_error("Unable to open /dev/gpiomem.");
		}

		while (!event_poll_thread_exit)
		{
			if (callback_map.empty())
			{
				// Wait untill callback_map is not empty
				std::unique_lock<std::mutex> lock(event_poll_mtx);
				event_poll_cond.wait(lock, [this] { return (!callback_map.empty() || !event_poll_thread_exit); });
			}
			else
			{
				poll(&mem, 1, -1);
				for (const auto& entry : callback_map)
				{
					_Reg reg_sel = entry.first / reg_size<_Reg>;
					_Reg event_occured = (*(base_event_reg + reg_sel) >> entry.first) & static_cast<_Reg>(1U);

					if (event_occured)
					{
						callback_queue.push(entry.second);
					}
				}
			}
		}

		close(mem.fd);
	}

	template<typename _Reg, typename _Fun>
	inline void gpio_callback_map<_Reg, _Fun>::push(std::pair<_Reg, _Fun>&& key_val)
	{
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			callback_map.insert(key_val);
		} // Release the lock and notify waiting thread
		event_poll_cond.notify_one();
	}
}