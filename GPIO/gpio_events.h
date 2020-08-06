#pragma once
#include <cstdint>
#include "gpio_predicates.h"

#if defined(BCM2711)
#include "bcm2711.h"
#else
static_assert(0, "No processor model was specified.");
#endif

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
			static constexpr __addr::reg_t offs = __addr::GPREN0;
		};

		// Falling edge event type.
		struct falling_edge
		{
			static constexpr __addr::reg_t offs = __addr::GPFEN0;
		};

		// Pin high event type.
		struct pin_high
		{
			static constexpr __addr::reg_t offs = __addr::GPHEN0;
		};

		// Pin low event type.
		struct pin_low
		{
			static constexpr __addr::reg_t offs = __addr::GPLEN0;
		};

		// Asynchronous rising edge event type.
		struct async_rising_edge
		{
			static constexpr __addr::reg_t offs = __addr::GPAREN0;
		};

		// Asynchronous falling edge event type.
		struct async_falling_edge
		{
			static constexpr __addr::reg_t offs = __addr::GPAFEN0;
		};
	}

	namespace __pred
	{
		/*
			Additional helper predicate for events.
		*/

		// Check if the specified type is an event.
		template<typename _Ty>
		inline constexpr bool __Is_event =
			__Is_same<_Ty, irq::rising_edge> ||
			__Is_same<_Ty, irq::falling_edge> ||
			__Is_same<_Ty, irq::pin_high> ||
			__Is_same<_Ty, irq::pin_low> ||
			__Is_same<_Ty, irq::async_rising_edge> ||
			__Is_same<_Ty, irq::async_falling_edge>;
	}
}