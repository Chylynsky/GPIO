#pragma once
namespace rpi4b
{
	template<typename _Reg>
	struct __gpio_output
	{
		volatile _Reg* set_reg;
		volatile _Reg* clr_reg;
	};
}