#pragma once
#include <cstdint>

namespace rpi::__impl::kernel
{
    struct command_t
    {
        std::uint32_t type;
        std::uint32_t pin_number;
    };

    inline constexpr std::uint32_t   CMD_DETACH_IRQ  = 0U;
    inline constexpr std::uint32_t   CMD_ATTACH_IRQ  = 1U;
    inline constexpr std::uint32_t   CMD_WAKE_UP     = 2U;
    inline constexpr std::size_t     COMMAND_SIZE    = sizeof(command_t);
}