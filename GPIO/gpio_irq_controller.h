#pragma once
#include <cassert>

#include "gpio_traits.h"
#include "kernel_interop.h"
#include "gpio_helper.h"
#include "gpio_irq_controller_base.h"

namespace rpi::__impl
{
    /*
        Class holding key - interval pairs where key is the GPIO pin number
        and interval is the entry function which gets executed when specified event
        occures. Its main task is to control whether an event has occured and if so,
        to pass it into dispatch queue.
    */
    class irq_controller : public irq_controller_base
    {
        std::unique_ptr<__impl::file_descriptor> driver;  // File descriptor used for driver interaction.

        void kernel_request_irq(const uint32_t pin);
        void kernel_irq_free(const uint32_t pin);
        void kernel_read_unblock();

    public:

        irq_controller();
        ~irq_controller();

        // Main event_poll_thread function.
        void poll_events() override;

        // Insert new key-interval pair.
        void request_irq(uint32_t pin, const callback_t& callback) override;

        // Erase all entry functions for the specified pin.
        void irq_free(uint32_t key) override;

        // No effect on driver based IRQ controller
        void set_poll_interval(std::chrono::nanoseconds value) override {}
    };

    void irq_controller::kernel_request_irq(const uint32_t pin)
    {
        kernel::command_t request{ kernel::CMD_ATTACH_IRQ, pin };
        ssize_t result = driver->write(&request, kernel::COMMAND_SIZE);

        if (result != kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("IRQ request failed.");
        }
    }

    void irq_controller::kernel_irq_free(const uint32_t pin)
    {
        kernel::command_t request{ kernel::CMD_DETACH_IRQ, pin };
        ssize_t result = driver->write(&request, kernel::COMMAND_SIZE);

        if (result != kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("IRQ free failed.");
        }
    }

    void irq_controller::kernel_read_unblock()
    {
        kernel::command_t request{ kernel::CMD_WAKE_UP, 0xFFFFU };
        ssize_t result = driver->write(&request, kernel::COMMAND_SIZE);

        if (result != kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("Read unblocking failed.");
        }
    }

    irq_controller::irq_controller()
    {
        try
        {
            driver = std::make_unique<__impl::file_descriptor>("/dev/gpiodev", O_RDWR);
        }
        catch (const std::runtime_error& err)
        {
            throw err;
        }
    }

    irq_controller::~irq_controller()
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

    void irq_controller::poll_events()
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

    void irq_controller::request_irq(uint32_t pin, const callback_t& callback)
    {
        try
        {
            kernel_request_irq(pin);
        }
        catch (const std::runtime_error& err)
        {
            throw err;
        }

        {
            std::lock_guard<std::mutex> lock{ event_poll_mtx };

            if (callback_map.empty())
            {
                event_poll_thread_exit = false;
                event_poll_thread = std::async(std::launch::async, [this]() { poll_events(); });
            }

            callback_map.insert(std::move(std::make_pair(pin, callback)));
        }
    }

    void irq_controller::irq_free(uint32_t key)
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