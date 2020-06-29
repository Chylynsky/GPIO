#pragma once
#include <type_traits>
#include "gpio_direction.h"

namespace rpi
{
	template<bool _Cond, typename _Ty = void>
	using __Enable_if = typename std::enable_if_t<_Cond, _Ty>;

	template<bool _Cond, typename Iftrue, typename Iffalse>
	using __Select_if = typename std::conditional_t<_Cond, Iftrue, Iffalse>;

	template<typename _Ty1, typename _Ty2>
	constexpr bool __Is_same = std::is_same_v<_Ty1, _Ty2>;

	template<typename _From, typename _To>
	constexpr bool __Is_convertible = std::is_convertible_v<_From, _To>;

	template<typename _Ty>
	constexpr bool __Is_integral = std::is_integral_v<_Ty>;

	template<typename _Dir>
	constexpr bool __Is_input = __Is_same<_Dir, dir::input>;

	template<typename _Dir>
	constexpr bool __Is_output = __Is_same<_Dir, dir::output>;

	template<typename _Dir>
	constexpr bool __Is_direction = 
		__Is_same<_Dir, dir::output> || 
		__Is_same<_Dir, dir::input>;
}