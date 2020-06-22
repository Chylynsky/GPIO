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

namespace rpi
{
	template<typename _Reg, typename _Fun>
	class gpio_callback_map
	{
		std::multimap<_Reg, _Fun> callback_map; // key - pin_number, value - callback function
		std::thread event_poll_thread;
		std::mutex event_poll_mtx;
		std::condition_variable event_poll_cond;
		bool event_poll_thread_exit;
		dispatch_queue<_Fun> callback_queue;

	public:

		gpio_callback_map();
		~gpio_callback_map();

		void poll_events();
		void insert(std::pair<_Reg, _Fun>&& key_value_pair);
		void erase(_Reg value);
	};

	template<typename _Reg, typename _Fun>
	inline gpio_callback_map<_Reg, _Fun>::gpio_callback_map() : event_poll_thread_exit{ false }
	{
		event_poll_thread = std::thread(std::bind(&gpio_callback_map<_Reg, _Fun>::poll_events, this));
	}

	template<typename _Reg, typename _Fun>
	inline gpio_callback_map<_Reg, _Fun>::~gpio_callback_map()
	{
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			event_poll_thread_exit = true;
		}
		event_poll_cond.notify_one();

		if (event_poll_thread.joinable())
		{
			event_poll_thread.join();
		}
	}

	template<typename _Reg, typename _Fun>
	inline void gpio_callback_map<_Reg, _Fun>::poll_events()
	{
		const std::string path = "/dev/gpiomem";
		volatile _Reg* base_event_reg = get_reg_ptr(addr::GPEDS0);

		pollfd mem;
		
		mem.fd = open(path.c_str(), O_RDONLY);
		mem.events = POLLIN;

		if (mem.fd < 0)
		{
			throw std::runtime_error("Unable to open " + path + ".");
		}

		while (!event_poll_thread_exit)
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			if (callback_map.empty())
			{
				// Wait untill callback_map is not empty
				event_poll_cond.wait(lock, [this] { return (!callback_map.empty() || event_poll_thread_exit); });
			}
			else
			{
				lock.unlock();
				poll(&mem, 1, 250);

				lock.lock();
				for (const auto& entry : callback_map)
				{
					_Reg pin_no = entry.first;
					_Reg reg_sel = pin_no / reg_size<_Reg>;
					_Reg event_occured = (*(base_event_reg + reg_sel) >> pin_no) & static_cast<_Reg>(1);

					if (event_occured)
					{
						// Clear event register
						*(base_event_reg + reg_sel) |= static_cast<_Reg>(1) << pin_no;

						_Fun callback = entry.second;
						callback_queue.push(callback);
					}
				}
			}
		}

		close(mem.fd);
	}

	template<typename _Reg, typename _Fun>
	inline void gpio_callback_map<_Reg, _Fun>::insert(std::pair<_Reg, _Fun>&& key_val)
	{
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			callback_map.insert(key_val);
		} // Release the lock and notify waiting thread
		event_poll_cond.notify_one();
	}

	template<typename _Reg, typename _Fun>
	inline void gpio_callback_map<_Reg, _Fun>::erase(_Reg value)
	{
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			callback_map.erase(value);
		} // Release the lock and notify waiting thread
		event_poll_cond.notify_one();
	}
}