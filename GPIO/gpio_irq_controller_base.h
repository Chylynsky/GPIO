#pragma once
#include <map>
#include <mutex>
#include <future>
#include <memory>
#include "gpio_aliases.h"
#include "dispatch_queue.h"

namespace rpi::__impl
{
    class irq_controller_base
    {
    protected:

        std::future<void>   event_poll_thread;      // Thread on which events are polled.
        std::mutex          event_poll_mtx;         // Mutex for resource access control.
        std::atomic<bool>   event_poll_thread_exit; // Loop control for event_poll_thread.

        std::multimap<uint32_t, callback_t>             callback_map;     // Multimap where key - pin_number, value - callback.
        std::unique_ptr<dispatch_queue<callback_t>>   callback_queue;   // When an event occurs, the corresponding entry function is pushed here.

    public:

        irq_controller_base();
        virtual ~irq_controller_base() {};

        // Main event_poll_thread function.
        virtual void poll_events() = 0;

        // Insert new key-value pair.
        virtual void request_irq(uint32_t pin, const callback_t& callback) = 0;

        // Erase all entry functions for the specified pin.
        virtual void irq_free(uint32_t key) = 0;

        // Set poll interval, no effect by default
        virtual void set_poll_interval(std::chrono::nanoseconds value) {}
    };
}