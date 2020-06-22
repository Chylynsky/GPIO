#pragma once

/*
	__gpio_output struct holds fields unique for gpio class input
	template type specialization.
*/

namespace rpi
{
	template<typename _Reg>
	struct __gpio_output
	{
		volatile _Reg* set_reg;
		volatile _Reg* clr_reg;
	};
}