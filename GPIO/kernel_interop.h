#pragma once

namespace rpi::__kernel
{
	struct command_t
	{
		unsigned char type;
		unsigned int pin_number;
	};

	inline constexpr unsigned char CMD_DETACH_IRQ = static_cast<unsigned char>(0);
	inline constexpr unsigned char CMD_ATTACH_IRQ = static_cast<unsigned char>(1);
	inline constexpr int COMMAND_SIZE = sizeof(command_t);
}