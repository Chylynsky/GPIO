#pragma once
#include <cstdint>
#include "bcm2711.h"
#include "gpio_predicates.h"

namespace rpi
{
	namespace irq
	{
		/*
			Each event type has 'value' field representing
			the base register used to turn on pin event detection
		*/

		struct rising_edge
		{
			static constexpr uint32_t value = addr::GPREN0;
		};

		struct falling_edge
		{
			static constexpr uint32_t value = addr::GPFEN0;
		};

		struct pin_high
		{
			static constexpr uint32_t value = addr::GPHEN0;
		};

		struct pin_low
		{
			static constexpr uint32_t value = addr::GPLEN0;
		};

		struct async_rising_edge
		{
			static constexpr uint32_t value = addr::GPAREN0;
		};

		struct async_falling_edge
		{
			static constexpr uint32_t value = addr::GPAFEN0;
		};
	}

	/*
		Additional helper predicates for events
	*/

	// Get base event register offset
	template<typename _Ev>
	constexpr uint32_t Event_reg_offs = _Ev::value;

	// Check if the specified type is event
	template<typename _Ty>
	constexpr bool Is_event = 
		Is_same<_Ty, irq::rising_edge>			|| 
		Is_same<_Ty, irq::falling_edge>			||
		Is_same<_Ty, irq::pin_high>				||
		Is_same<_Ty, irq::pin_low>				||
		Is_same<_Ty, irq::async_rising_edge>	||
		Is_same<_Ty, irq::async_falling_edge>;
}