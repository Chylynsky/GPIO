#pragma once
#include <cassert>

#include "gpio_predicates.h"
#include "kernel_interop.h"
#include "gpio_helper.h"
#include "gpio_irq_controller_base.h"

#if defined(BCM2711)
#include "bcm2711.h"
#else
static_assert(0, "Processor model must be specified as a macro definition before including gpio.h.");
#endif

namespace rpi
{
    namespace irq_mode
    {
        struct polled{};
        struct driver{};
    }

    namespace __pred
    {
        template<typename _Ty>
        inline constexpr bool __Is_polled = __Is_same<_Ty, irq_mode::polled>;

        template<typename _Ty>
        inline constexpr bool __Is_driver = __Is_same<_Ty, irq_mode::driver>;

        template<typename _Ty>
        inline constexpr bool __Is_irq_mode = __Is_polled<_Ty> || __Is_driver<_Ty>;
    }

    template<typename _Ty>
    class __irq_controller 
    { 
        static_assert(__pred::__Is_irq_mode<_Ty>, "Bad IRQ mode.");
    };

    /*
        Class holding key - interval pairs where key is the GPIO pin number
        and interval is the entry function which gets executed when specified event
        occures. Its main task is to control whether an event has occured and if so,
        to pass it into dispatch queue.
    */
    template<>
    class __irq_controller<irq_mode::driver> : public __irq_controller_base
    {
        std::unique_ptr<__file_descriptor> driver;  // File descriptor used for driver interaction.

        void kernel_request_irq(const uint32_t pin);
        void kernel_irq_free(const uint32_t pin);
        void kernel_read_unblock();

    public:

        __irq_controller();
        ~__irq_controller();

        // Main event_poll_thread function.
        void poll_events() override;

        // Insert new key-interval pair.
        void request_irq(uint32_t pin, const callback_t& callback) override;

        // Erase all entry functions for the specified pin.
        void irq_free(uint32_t key) override;

        // No effect on driver based IRQ controller
        void set_poll_interval(std::chrono::nanoseconds value) override {}
    };

    template<>
    class __irq_controller<irq_mode::polled> : public __irq_controller_base
    {
        std::chrono::nanoseconds poll_interval;

    public:

        __irq_controller();
        ~__irq_controller();

        // Main event_poll_thread function.
        void poll_events() override;

        // Insert new key-interval pair.
        void request_irq(uint32_t pin, const callback_t& callback) override;

        // Erase all entry functions for the specified pin.
        void irq_free(uint32_t key) override;

        void set_poll_interval(std::chrono::nanoseconds value) override;
    };

    void __irq_controller<irq_mode::driver>::kernel_request_irq(const uint32_t pin)
    {
        __kernel::command_t request{ __kernel::CMD_ATTACH_IRQ, pin };
        ssize_t result = driver->write(&request, __kernel::COMMAND_SIZE);

        if (result != __kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("IRQ request failed.");
        }
    }

    void __irq_controller<irq_mode::driver>::kernel_irq_free(const uint32_t pin)
    {
        __kernel::command_t request{ __kernel::CMD_DETACH_IRQ, pin };
        ssize_t result = driver->write(&request, __kernel::COMMAND_SIZE);

        if (result != __kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("IRQ free failed.");
        }
    }

    void __irq_controller<irq_mode::driver>::kernel_read_unblock()
    {
        __kernel::command_t request{ __kernel::CMD_WAKE_UP, 0xFFFFU };
        ssize_t result = driver->write(&request, __kernel::COMMAND_SIZE);

        if (result != __kernel::COMMAND_SIZE)
        {
            throw std::runtime_error("Read unblocking failed.");
        }
    }

    __irq_controller<irq_mode::driver>::__irq_controller()
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

    __irq_controller<irq_mode::driver>::~__irq_controller()
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

    void __irq_controller<irq_mode::driver>::poll_events()
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

    void __irq_controller<irq_mode::driver>::request_irq(uint32_t pin, const callback_t& callback)
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

    void __irq_controller<irq_mode::driver>::irq_free(uint32_t key)
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

    __irq_controller<irq_mode::polled>::__irq_controller() : 
        poll_interval{ std::chrono::nanoseconds(1000) }
    {
    }

    __irq_controller<irq_mode::polled>::~__irq_controller()
    {
    }

    void __irq_controller<irq_mode::polled>::poll_events()
    {
        using namespace std::this_thread;
        using namespace std::chrono_literals;

        while (!event_poll_thread_exit)
        {


            sleep_for(poll_interval);
        }
    }

    void __irq_controller<irq_mode::polled>::request_irq(uint32_t pin, const callback_t& callback)
    {
        std::lock_guard<std::mutex> lock{ event_poll_mtx };

        if (callback_map.empty())
        {
            event_poll_thread_exit = false;
            event_poll_thread = std::async(std::launch::async, [this]() { poll_events(); });
        }

        callback_map.insert(std::move(std::make_pair(pin, callback)));
    }

    void __irq_controller<irq_mode::polled>::irq_free(uint32_t key)
    {
        {
            std::lock_guard<std::mutex> lock{ event_poll_mtx };
            callback_map.erase(key);

            if (!callback_map.empty())
            {
                return;
            }
        }

        event_poll_thread_exit = true;

        if (event_poll_thread.valid())
        {
            event_poll_thread.wait();
        }
    }

    inline void __irq_controller<irq_mode::polled>::set_poll_interval(std::chrono::nanoseconds interval)
    {
        poll_interval = interval;
    }
}