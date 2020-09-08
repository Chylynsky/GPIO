#include <utility>
#include <cassert>
#include <exception>
#include <stdexcept>
#include "kernel_interop.h"
#include "gpio_callback_map.h"

namespace rpi
{
	void __irq_controller::request_irq(const uint32_t pin)
	{
		__kernel::command_t request{ __kernel::CMD_ATTACH_IRQ, pin };
		ssize_t result = driver->write(&request, __kernel::COMMAND_SIZE);

		if (result != __kernel::COMMAND_SIZE)
		{
			throw std::runtime_error("IRQ request failed.");
		}
	}

	void __irq_controller::free_irq(const uint32_t pin)
	{
		__kernel::command_t request{ __kernel::CMD_DETACH_IRQ, pin };
		ssize_t result = driver->write(&request, __kernel::COMMAND_SIZE);

		if (result != __kernel::COMMAND_SIZE)
		{
			throw std::runtime_error("IRQ request failed.");
		}
	}

	__irq_controller::__irq_controller() : event_poll_thread_exit{ false }
	{
		try
		{
			driver = std::make_unique<__file_descriptor>("/dev/gpiodev", O_RDWR);
		}
		catch (const std::runtime_error& err)
		{
			throw err;
		}

		event_poll_thread = std::async(std::launch::async, [this]() { poll_events(); });
	}

	__irq_controller::~__irq_controller()
	{
		driver.release();
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);

			for (auto& entry : irq_controller)
			{
				try
				{
					free_irq(entry.first);
				}
				catch (const std::runtime_error& err)
				{
					assert(0 && "IRQ not found!");
					continue;
				}
			}

			irq_controller.clear();
			event_poll_thread_exit = true;
		} // Release the lock and notify waiting thread.
		event_poll_cond.notify_one();

		event_poll_thread.get();
	}

	void __irq_controller::poll_events()
	{
		while (!event_poll_thread_exit)
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			if (irq_controller.empty())
			{
				// Wait untill irq_controller is not empty.
				event_poll_cond.wait(lock, [this]() { return (!irq_controller.empty() || event_poll_thread_exit); });
			}
			else
			{
				uint32_t pin;
				lock.unlock(); // Allow for device release driver while waiting for data
				if (driver->read(&pin, sizeof(pin)) == sizeof(pin))
				{
					lock.lock();
					auto entry = irq_controller.find(pin);
					callback_queue.push((*entry).second);
				}
			}
		}
	}

	void __irq_controller::insert(uint32_t pin, const callback_t& callback)
	{
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);

			try
			{
				request_irq(pin);
			}
			catch (const std::runtime_error& err)
			{
				lock.unlock();
				event_poll_cond.notify_one();
				throw err;
			}

			irq_controller.insert(std::move(std::make_pair(pin, callback)));
		} // Release the lock and notify waiting thread.
		event_poll_cond.notify_one();
	}

	void __irq_controller::erase(uint32_t key)
	{
		{
			std::unique_lock<std::mutex> lock(event_poll_mtx);
			irq_controller.erase(key);

			try
			{
				free_irq(key);
			}
			catch (const std::runtime_error& err)
			{
				lock.unlock();
				event_poll_cond.notify_one();
				throw err;
			}

		} // Release the lock and notify waiting thread.
		event_poll_cond.notify_one();
	}
}