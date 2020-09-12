#pragma once
#include <map>
#include <mutex>
#include <future>
#include <memory>
#include <condition_variable>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <poll.h>

#include "gpio_aliases.h"
#include "gpio_helper.h"
#include "dispatch_queue.h"

namespace rpi
{
    /*
        Class holding key - value pairs where key is the GPIO pin number
        and value is the entry function which gets executed when specified event
        occures. Its main task is to control whether an event has occured and if so,
        to pass it into dispatch queue.
    */
    class __irq_controller
    {
        std::future<void>       event_poll_thread;      // Thread on which events are polled.
        std::mutex              event_poll_mtx;         // Mutex for resource access control.
        std::condition_variable event_poll_cond;        // Puts the thread to sleep when irq_controller is empty.
        std::atomic<bool>       event_poll_thread_exit; // Loop control for event_poll_thread.

        std::multimap<uint32_t, callback_t> callback_map;               // Multimap where key - pin_number, value - entry function.

        std::unique_ptr <__file_descriptor>             driver;         // File descriptor used for driver interaction.
        std::unique_ptr<__dispatch_queue<callback_t>>   callback_queue; // When an event occurs, the corresponding entry function is pushed here.

        void kernel_request_irq(const uint32_t pin);
        void kernel_irq_free(const uint32_t pin);
        void kernel_read_unblock();

    public:

        __irq_controller();
        ~__irq_controller();

        // Main event_poll_thread function.
        void poll_events();

        // Insert new key-value pair.
        void request_irq(uint32_t pin, const callback_t& callback);

        // Erase all entry functions for the specified pin.
        void irq_free(uint32_t key);
    };
}