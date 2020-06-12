#pragma once
#include "bcm2711.h"
#include "gpio_predicates.h"
#include <cstdint>

namespace rpi4b
{
	// Each event type has 'value' field representing
	// the base register used to turn on pin event detection

	struct rising_edge
	{
		static constexpr uint32_t value = GPREN0;
	};

	struct falling_edge
	{
		static constexpr uint32_t value = GPFEN0;
	};

	struct pin_high
	{
		static constexpr uint32_t value = GPHEN0;
	};

	struct pin_low
	{
		static constexpr uint32_t value = GPLEN0;
	};

	struct async_rising_edge
	{
		static constexpr uint32_t value = GPAREN0;
	};

	struct async_falling_edge
	{
		static constexpr uint32_t value = GPAFEN0;
	};

	template<typename _Ev>
	constexpr uint32_t Event_reg_offs = _Ev::value;

	template<typename _Ty>
	constexpr bool Is_event = 
		Is_same<_Ty, rising_edge>			|| 
		Is_same<_Ty, falling_edge>			|| 
		Is_same<_Ty, pin_high>				|| 
		Is_same<_Ty, pin_low>				|| 
		Is_same<_Ty, async_rising_edge>		|| 
		Is_same<_Ty, async_falling_edge>;
}