#pragma once

/*
    gpio_output struct holds fields unique for gpio class input
    template type specialization.
*/

namespace rpi::__impl
{
    template<typename _Reg>
    struct gpio_output
    {
        volatile _Reg* set_reg;
        volatile _Reg* clr_reg;
    };
}