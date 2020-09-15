#include "gpio_irq_controller_base.h"

namespace rpi
{
    __irq_controller_base::__irq_controller_base() :
        event_poll_thread_exit{ false },
        callback_queue{ std::make_unique<__dispatch_queue<callback_t>>() }
    {
    }
}