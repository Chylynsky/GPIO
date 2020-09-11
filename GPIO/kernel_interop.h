#pragma once

namespace rpi::__kernel
{
    struct command_t
    {
        uint32_t type;
        uint32_t pin_number;
    };

    inline constexpr uint32_t	CMD_DETACH_IRQ	= 0U;
    inline constexpr uint32_t	CMD_ATTACH_IRQ	= 1U;
    inline constexpr uint32_t	CMD_WAKE_UP		= 2U;
    inline constexpr size_t		COMMAND_SIZE	= sizeof(command_t);
}