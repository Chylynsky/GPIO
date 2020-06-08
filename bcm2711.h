#pragma once
#include <cstdint>
#include <sys/mman.h>

// BCM2711 documentation:
// https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2711/rpi_DATA_2711_1p0.pdf

namespace rpi4b
{
	inline constexpr uintptr_t GPIO_REGISTER_BASE	{ 0x7E215000 };	// GPIO register base address

	// GPIO function select
	inline constexpr uintptr_t GPFSEL0	{ GPIO_REGISTER_BASE };			// GPIO function select 0
	inline constexpr uintptr_t GPFSEL1	{ GPIO_REGISTER_BASE + 0x04 };	// GPIO function select 1
	inline constexpr uintptr_t GPFSEL2	{ GPIO_REGISTER_BASE + 0x08 };  // GPIO function select 2
	inline constexpr uintptr_t GPFSEL3	{ GPIO_REGISTER_BASE + 0x0C };  // GPIO function select 3
	inline constexpr uintptr_t GPFSEL4	{ GPIO_REGISTER_BASE + 0x10 };  // GPIO function select 4
	inline constexpr uintptr_t GPFSEL5	{ GPIO_REGISTER_BASE + 0x14 };  // GPIO function select 5

	// Number of bits that every pin occupies in gpfsel registers
	inline constexpr uint32_t GPFSEL_PIN_BIT_SIZE	{ 3 };

	// Values for GPFSEL registers
	enum class GPIOFunctionSelect
	{
		GPIO_PIN_AS_INPUT = 0b000,
		GPIO_PIN_AS_OUTPUT = 0b001,
		GPIO_PIN_ALTERNATE_FUNCTION_0 = 0b100,
		GPIO_PIN_ALTERNATE_FUNCTION_1 = 0b101,
		GPIO_PIN_ALTERNATE_FUNCTION_2 = 0b110,
		GPIO_PIN_ALTERNATE_FUNCTION_3 = 0b111,
		GPIO_PIN_ALTERNATE_FUNCTION_4 = 0b011,
		GPIO_PIN_ALTERNATE_FUNCTION_5 = 0b010
	};

	// GPIO pin output set
	inline constexpr uintptr_t GPSET0	{ GPIO_REGISTER_BASE + 0x1C };	// GPIO pin output set 0
	inline constexpr uintptr_t GPSET1	{ GPIO_REGISTER_BASE + 0x20 };	// GPIO pin output set 1

	enum class GPIOSetState
	{
		LOW = 0,
		HIGH = 1
	};

	// GPIO pin output clear
	inline constexpr uintptr_t GPCLR0	{ GPIO_REGISTER_BASE + 0x28 };	// GPIO pin output clear 0
	inline constexpr uintptr_t GPCLR1	{ GPIO_REGISTER_BASE + 0x2C };	// GPIO pin output clear 1

	// GPIO pin level
	inline constexpr uintptr_t GPLEV0	{ GPIO_REGISTER_BASE + 0x34 };	// GPIO pin level 0
	inline constexpr uintptr_t GPLEV1	{ GPIO_REGISTER_BASE + 0x38 };	// GPIO pin level 1

	// GPIO pin event detect status
	inline constexpr uintptr_t GPEDS0	{ GPIO_REGISTER_BASE + 0x40 };	// GPIO pin event detect status 0
	inline constexpr uintptr_t GPEDS1	{ GPIO_REGISTER_BASE + 0x44 };	// GPIO pin event detect status 1

	// GPIO pin rising edge detect enable
	inline constexpr uintptr_t GPREN0	{ GPIO_REGISTER_BASE + 0x4C };	// GPIO pin rising edge detect enable 0
	inline constexpr uintptr_t GPREN1	{ GPIO_REGISTER_BASE + 0x50 };	// GPIO pin rising edge detect enable 1

	// GPIO pin falling edge detect enable
	inline constexpr uintptr_t GPFEN0	{ GPIO_REGISTER_BASE + 0x58 };	// GPIO pin falling edge detect enable 0
	inline constexpr uintptr_t GPFEN1	{ GPIO_REGISTER_BASE + 0x5C };	// GPIO pin falling edge detect enable 1

	// GPIO pin high detect enable
	inline constexpr uintptr_t GPHEN0	{ GPIO_REGISTER_BASE + 0x64 };	// GPIO pin high detect enable 0
	inline constexpr uintptr_t GPHEN1	{ GPIO_REGISTER_BASE + 0x68 };	// GPIO pin high detect enable 1

	// GPIO pin low detect enable
	inline constexpr uintptr_t GPLEN0	{ GPIO_REGISTER_BASE + 0x70 };	// GPIO pin low detect enable 1
	inline constexpr uintptr_t GPLEN1	{ GPIO_REGISTER_BASE + 0x74 };	// GPIO pin low detect enable 1

	// GPIO pin async rising edge detect
	inline constexpr uintptr_t GPAREN0	{ GPIO_REGISTER_BASE + 0x7C };	// GPIO pin async rising edge detect 0
	inline constexpr uintptr_t GPAREN1	{ GPIO_REGISTER_BASE + 0x80 };	// GPIO pin async rising edge detect 1

	// GPIO pin async falling edge detect
	inline constexpr uintptr_t GPAFEN0	{ GPIO_REGISTER_BASE + 0x88 };	// GPIO pin async falling edge detect 0
	inline constexpr uintptr_t GPAFEN1	{ GPIO_REGISTER_BASE + 0x8C };	// GPIO pin async falling edge detect 1

	// GPIO pull-up/pull-down registers
	inline constexpr uintptr_t GPIO_PUP_PDN_CNTRL_REG0	{ GPIO_REGISTER_BASE + 0xE4 };	// GPIO pull-up/pull-down register 0
	inline constexpr uintptr_t GPIO_PUP_PDN_CNTRL_REG1	{ GPIO_REGISTER_BASE + 0xE8 };	// GPIO pull-up/pull-down register 1
	inline constexpr uintptr_t GPIO_PUP_PDN_CNTRL_REG2	{ GPIO_REGISTER_BASE + 0xEC };	// GPIO pull-up/pull-down register 2
	inline constexpr uintptr_t GPIO_PUP_PDN_CNTRL_REG3	{ GPIO_REGISTER_BASE + 0xF0 };	// GPIO pull-up/pull-down register 3
}