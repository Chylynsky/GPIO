#include "gpio_irq_controller.h"

namespace rpi::__impl
{
    void irq_controller::kernel_request_irq(const uint32_t gpio_number)
    {
        kernel::command_t request{ kernel::CMD_ATTACH_IRQ, gpio_number };
        ssize_t result = driver->write(&request, kernel::COMMAND_SIZE);

        if (result != kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("IRQ request failed.");
        }
    }

    void irq_controller::kernel_irq_free(const uint32_t gpio_number)
    {
        kernel::command_t request{ kernel::CMD_DETACH_IRQ, gpio_number };
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
        uint32_t gpio_number = 0U;

        while (!event_poll_thread_exit)
        {
            if (driver->read(&gpio_number, sizeof(gpio_number)) != sizeof(gpio_number))
            {
                continue;
            }

            std::unique_lock<std::mutex> lock{ event_poll_mtx };
            auto entry = callback_map.find(gpio_number);

            if (entry != callback_map.end())
            {
                lock.unlock();
                callback_queue->push((*entry).second);
                continue;
            }
        }
    }

    void irq_controller::request_irq(uint32_t gpio_number, const callback_t& callback)
    {
        try
        {
            kernel_request_irq(gpio_number);
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

            callback_map.insert(std::move(std::make_pair(gpio_number, callback)));
        }
    }

    void irq_controller::irq_free(uint32_t gpio_number)
    {
        try
        {
            kernel_irq_free(gpio_number);
        }
        catch (const std::runtime_error& err)
        {
            throw err;
        }

        {
            std::lock_guard<std::mutex> lock{ event_poll_mtx };
            callback_map.erase(gpio_number);

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