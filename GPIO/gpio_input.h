#pragma once
#include <list>
#include <memory>
#include <cstdint>
#include "gpio_aliases.h"
#include "gpio_irq_controller.h"

namespace rpi::__impl
{
    /*
        gpio_input struct holds fields unique for gpio class input
        template type specialization.
    */

    template<typename _Reg>
    struct gpio_input
    {
        static std::unique_ptr<irq_controller_base> irq_controller;
        static uint32_t irqs_set;

        std::list<volatile _Reg*>   event_regs_used;
        volatile _Reg*              level_reg;
    };

    template<typename _Reg>
    uint32_t gpio_input<_Reg>::irqs_set{ 0U };

    template<typename _Reg>
    std::unique_ptr<irq_controller_base> gpio_input<_Reg>::irq_controller{ nullptr };
}