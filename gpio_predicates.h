#pragma once
#include <type_traits>

namespace rpi4b
{
	template<bool _Cond, typename _Ty = void>
	using Enable_if = typename std::enable_if_t<_Cond, _Ty>;

	template<bool _Cond, typename Iftrue, typename Iffalse>
	using Select_if = typename std::conditional_t<_Cond, Iftrue, Iffalse>;

	template<typename _Ty1, typename _Ty2>
	inline constexpr bool Is_same = std::is_same_v<_Ty1, _Ty2>;

	template<typename _From, typename _To>
	inline constexpr bool Is_convertible = std::is_convertible_v<_From, _To>;

	struct input
	{};

	struct output
	{};

	template<typename _Dir>
	inline constexpr bool Is_input = Is_same<_Dir, input>;

	template<typename _Dir>
	inline constexpr bool Is_output = Is_same<_Dir, output>;
}