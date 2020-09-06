#pragma once
#include <list>
#include "gpio_aliases.h"
#include "gpio_callback_map.h"

namespace rpi
{
	/*
		__gpio_input struct holds fields unique for gpio class input
		template type specialization.
	*/

	template<typename _Reg>
	struct __gpio_input
	{
		static __gpio_callback_map<_Reg, callback_t> callback_map;
		std::list<volatile _Reg*> event_regs_used;
		volatile _Reg* lev_reg;
	};

	template<typename _Reg>
	__gpio_callback_map<_Reg, callback_t> __gpio_input<_Reg>::callback_map{};
}