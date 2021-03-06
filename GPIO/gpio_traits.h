#pragma once
#include <type_traits>
#include "gpio_direction.h"

namespace rpi::__impl::traits
{
    template<bool _Cond, typename _Ty = void>
    using Enable_if = typename std::enable_if_t<_Cond, _Ty>;

    template<bool _Cond, typename Iftrue, typename Iffalse>
    using Select_if = typename std::conditional_t<_Cond, Iftrue, Iffalse>;

    template<bool _Cond, typename Iftrue, typename Iffalse>
    using Derive_if = Select_if<_Cond, Iftrue, Iffalse>;

    template<typename _Ty1, typename _Ty2>
    inline constexpr bool Is_same = std::is_same_v<_Ty1, _Ty2>;

    template<typename _From, typename _To>
    inline constexpr bool Is_convertible = std::is_convertible_v<_From, _To>;

    template<typename _Ty>
    inline constexpr bool Is_intergral = std::is_integral_v<_Ty>;

    template<typename _Dir>
    inline constexpr bool Is_input = Is_same<_Dir, dir::input>;

    template<typename _Dir>
    inline constexpr bool Is_output = Is_same<_Dir, dir::output>;

    template<typename _Dir>
    inline constexpr bool Is_direction = 
        Is_input<_Dir> ||
        Is_output<_Dir>;

    template<typename _Arg>
    inline constexpr bool Is_constant =
        Is_same<_Arg, std::true_type> ||
        Is_same<_Arg, std::false_type>;
}