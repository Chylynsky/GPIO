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
            throw std::runtime_error("IRQ free failed.");
        }
    }

    void __irq_controller::kernel_read_unblock()
    {
        __kernel::command_t request{ __kernel::CMD_WAKE_UP, 0xFFFFU };
        ssize_t result = driver->write(&request, __kernel::COMMAND_SIZE);

        if (result != __kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("Read unblocking failed.");
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
    }

    __irq_controller::~__irq_controller()
    {
        event_poll_thread_exit = true;
        kernel_read_unblock();

        if (event_poll_thread.valid())
        {
            event_poll_thread.wait();
        }
        
        // Destroy callback queue to avoid calling a dangling reference to a function object
        callback_queue.reset();

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
            if (driver->read(&pin, sizeof(pin)) != sizeof(pin))
            {
                continue;
            }

            std::unique_lock<std::mutex> lock{ event_poll_mtx };
            auto entry = callback_map.find(pin);

            if (entry != callback_map.end())
            {
                lock.unlock();
                callback_queue->push((*entry).second);
                continue;
            }
        }
    }

    void __irq_controller::request_irq(uint32_t pin, const callback_t& callback)
    {
        {
            std::lock_guard<std::mutex> lock{ event_poll_mtx };

            if (callback_map.empty())
            {
                event_poll_thread_exit = false;
                event_poll_thread = std::async(std::launch::async, [this]() { poll_events(); });
            }

            callback_map.insert(std::move(std::make_pair(pin, callback)));
        }

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
        try
        {
            kernel_irq_free(key);
        }
        catch (const std::runtime_error& err)
        {
            throw err;
        }

        {
            std::lock_guard<std::mutex> lock{ event_poll_mtx };
            callback_map.erase(key);

            if (!callback_map.empty())
            {
                return;
            }
        }

        event_poll_thread_exit = true;
        kernel_read_unblock();
                
        if (event_poll_thread.valid())
        {
            event_poll_thread.wait();
        }
    }
}