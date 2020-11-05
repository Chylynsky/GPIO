#pragma once
#include <cstdint>

#include "gpio_traits.h"
#include "bcm2711.h"

namespace rpi
{
    namespace irq
    {
        /*
            Each event type has an 'offs' field representing
            the base register used to turn on pin event detection.
        */

        // Rising edge event type.
        struct rising_edge
        {
            static constexpr reg_t offs = __impl::addr::GPREN0;
        };

        // Falling edge event type.
        struct falling_edge
        {
            static constexpr reg_t offs = __impl::addr::GPFEN0;
        };

        // Pin high event type.
        struct pin_high
        {
            static constexpr reg_t offs = __impl::addr::GPHEN0;
        };

        // Pin low event type.
        struct pin_low
        {
            static constexpr reg_t offs = __impl::addr::GPLEN0;
        };

        // Asynchronous rising edge event type.
        struct async_rising_edge
        {
            static constexpr reg_t offs = __impl::addr::GPAREN0;
        };

        // Asynchronous falling edge event type.
        struct async_falling_edge
        {
            static constexpr reg_t offs = __impl::addr::GPAFEN0;
        };
    }

    namespace __impl::traits
    {
        /*
            Additional helper predicate for events.
        */

        // Check if the specified type is an event.
        template<typename _Ty>
        inline constexpr bool Is_event =
            Is_same<_Ty, irq::rising_edge>        ||
            Is_same<_Ty, irq::falling_edge>       ||
            Is_same<_Ty, irq::pin_high>           ||
            Is_same<_Ty, irq::pin_low>            ||
            Is_same<_Ty, irq::async_rising_edge>  ||
            Is_same<_Ty, irq::async_falling_edge>;
    }
}