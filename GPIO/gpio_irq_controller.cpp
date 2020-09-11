#include <utility>
#include <cassert>
#include <exception>
#include <stdexcept>
#include "kernel_interop.h"
#include "gpio_irq_controller.h"

namespace rpi
{
    void __irq_controller::kernel_request_irq(const uint32_t pin)
    {
        __kernel::command_t request{ __kernel::CMD_ATTACH_IRQ, pin };
        ssize_t result = driver->write(&request, __kernel::COMMAND_SIZE);

        if (result != __kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("IRQ request failed.");
        }
    }

    void __irq_controller::kernel_irq_free(const uint32_t pin)
    {
        __kernel::command_t request{ __kernel::CMD_DETACH_IRQ, pin };
        ssize_t result = driver->write(&request, __kernel::COMMAND_SIZE);

        if (result != __kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("IRQ request failed.");
        }
    }

    void __irq_controller::kernel_read_unblock()
    {
        __kernel::command_t request{ __kernel::CMD_WAKE_UP, 0xFFFFU };
        ssize_t result = driver->write(&request, __kernel::COMMAND_SIZE);

        if (result != __kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("IRQ request failed.");
        }
    }

    __irq_controller::__irq_controller() :
        event_poll_thread_exit{ false },
        callback_queue{ std::make_unique<__dispatch_queue<callback_t>>() }
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
        event_poll_thread_exit = true;
        event_poll_cond.notify_one();
        kernel_read_unblock();
        event_poll_thread.wait();
        callback_queue.reset();	// Destroy callback queue to avoid calling a dangling reference to a function object

        for (auto& entry : callback_map)
        {
            try
            {
                kernel_irq_free(entry.first);
            }
            catch (const std::runtime_error& err)
            {
                assert(0 && "IRQ not found!");
                continue;
            }
        }

        callback_map.clear();
    }

    void __irq_controller::poll_events()
    {
        uint32_t pin = 0U;

        while (!event_poll_thread_exit)
        {
            std::unique_lock<std::mutex> lock{ event_poll_mtx };
            if (callback_map.empty())
            {
                // Wait untill irq_controller is not empty.
                event_poll_cond.wait(lock, [this]() { return (!callback_map.empty() || event_poll_thread_exit); });
                continue;
            }

            lock.unlock();
            if (driver->read(&pin, sizeof(pin)) != sizeof(pin))
            {
                continue;
            }

            lock.lock();
            auto entry = callback_map.find(pin);

            if (entry != callback_map.end())
            {
                lock.unlock();
                callback_queue->push((*entry).second);
                continue;
            }

            lock.unlock();
        }
    }

    void __irq_controller::request_irq(uint32_t pin, const callback_t& callback)
    {
        {
            std::lock_guard<std::mutex> lock{ event_poll_mtx };
            callback_map.request_irq(std::move(std::make_pair(pin, callback)));
        } // Release the lock and notify waiting thread.
        event_poll_cond.notify_one();

        try
        {
            kernel_request_irq(pin);
        }
        catch (const std::runtime_error& err)
        {
            throw err;
        }
    }

    void __irq_controller::irq_free(uint32_t key)
    {
        {
            std::lock_guard<std::mutex> lock{ event_poll_mtx };
            callback_map.irq_free(key);
        } // Release the lock and notify waiting thread.
        event_poll_cond.notify_one();

        try
        {
            kernel_irq_free(key);
        }
        catch (const std::runtime_error& err)
        {
            throw err;
        }
    }
}