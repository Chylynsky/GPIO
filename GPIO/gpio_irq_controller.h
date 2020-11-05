#pragma once
#include <cassert>

#include "gpio_traits.h"
#include "kernel_interop.h"
#include "gpio_helper.h"
#include "gpio_irq_controller_base.h"

namespace rpi::__impl
{
    /*
        Class holding key - interval pairs where key is the GPIO gpio_number number
        and interval is the entry function which gets executed when specified event
        occures. Its main task is to control whether an event has occured and if so,
        to pass it into dispatch queue.
    */
    class irq_controller : public irq_controller_base
    {
        std::unique_ptr<__impl::file_descriptor> driver;  // File descriptor used for driver interaction.

        void kernel_request_irq(const uint32_t gpio_number);
        void kernel_irq_free(const uint32_t gpio_number);
        void kernel_read_unblock();

    public:

        irq_controller();
        virtual ~irq_controller();

        // Main event_poll_thread function.
        void poll_events() override;

        // Insert new key-interval pair.
        void request_irq(uint32_t gpio_number, const callback_t& callback) override;

        // Erase all entry functions for the specified gpio_number.
        void irq_free(uint32_t gpio_number) override;
    };
}