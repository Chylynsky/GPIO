#pragma once
#include <cstdint>
#include "bcm2711.h"
#include "gpio_predicates.h"

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
			static constexpr uint32_t offs = __addr::GPREN0;
		};

		// Falling edge event type.
		struct falling_edge
		{
			static constexpr uint32_t offs = __addr::GPFEN0;
		};

		// Pin high event type.
		struct pin_high
		{
			static constexpr uint32_t offs = __addr::GPHEN0;
		};

		// Pin low event type.
		struct pin_low
		{
			static constexpr uint32_t offs = __addr::GPLEN0;
		};

		// Asynchronous rising edge event type.
		struct async_rising_edge
		{
			static constexpr uint32_t offs = __addr::GPAREN0;
		};

		// Asynchronous falling edge event type.
		struct async_falling_edge
		{
			static constexpr uint32_t offs = __addr::GPAFEN0;
		};
	}

	/*
		Additional helper predicates for events.
	*/

	// Get base event register offs.
	template<typename _Ev>
	inline constexpr uint32_t __Event_reg_offs = _Ev::offs;

	// Check if the specified type is an event.
	template<typename _Ty>
	inline constexpr bool __Is_event = 
		__Is_same<_Ty, irq::rising_edge>			||
		__Is_same<_Ty, irq::falling_edge>			||
		__Is_same<_Ty, irq::pin_high>				||
		__Is_same<_Ty, irq::pin_low>				||
		__Is_same<_Ty, irq::async_rising_edge>		||
		__Is_same<_Ty, irq::async_falling_edge>;
}