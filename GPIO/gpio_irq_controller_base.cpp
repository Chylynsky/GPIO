#include "gpio_irq_controller_base.h"

namespace rpi::__impl
{
    irq_controller_base::irq_controller_base() :
        event_poll_thread_exit{ false },
        callback_queue{ std::make_unique<dispatch_queue<callback_t>>() }
    {
    }
}