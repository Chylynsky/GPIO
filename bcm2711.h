#pragma once
#include <cstdint>

/*
	BCM2711 documentation:
	https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2711/rpi_DATA_2711_1p0.pdf
*/

namespace rpi
{
	using reg_t = uint32_t;

	namespace __addr
	{
		// GPIO function select offsets
		constexpr reg_t GPFSEL0{ 0x00U / sizeof(reg_t) };	// GPIO function select 0
		constexpr reg_t GPFSEL1{ 0x04U / sizeof(reg_t) };	// GPIO function select 1
		constexpr reg_t GPFSEL2{ 0x08U / sizeof(reg_t) };	// GPIO function select 2
		constexpr reg_t GPFSEL3{ 0x0CU / sizeof(reg_t) };	// GPIO function select 3
		constexpr reg_t GPFSEL4{ 0x10U / sizeof(reg_t) };	// GPIO function select 4
		constexpr reg_t GPFSEL5{ 0x14U / sizeof(reg_t) };	// GPIO function select 5

		// GPIO pin output set offsets
		constexpr reg_t GPSET0{ 0x1CU / sizeof(reg_t) };	// GPIO pin output set 0
		constexpr reg_t GPSET1{ 0x20U / sizeof(reg_t) };	// GPIO pin output set 1

		// GPIO pin output clear offsets
		constexpr reg_t GPCLR0{ 0x28U / sizeof(reg_t) };	// GPIO pin output clear 0
		constexpr reg_t GPCLR1{ 0x2CU / sizeof(reg_t) };	// GPIO pin output clear 1

		// GPIO pin level offsets
		constexpr reg_t GPLEV0{ 0x34U / sizeof(reg_t) };	// GPIO pin level 0
		constexpr reg_t GPLEV1{ 0x38U / sizeof(reg_t) };	// GPIO pin level 1

		// GPIO pin event detect status offsets
		constexpr reg_t GPEDS0{ 0x40U / sizeof(reg_t) };	// GPIO pin event detect status 0
		constexpr reg_t GPEDS1{ 0x44U / sizeof(reg_t) };	// GPIO pin event detect status 1

		// GPIO pin rising edge detect enable offsets
		constexpr reg_t GPREN0{ 0x4CU / sizeof(reg_t) };	// GPIO pin rising edge detect enable 0
		constexpr reg_t GPREN1{ 0x50U / sizeof(reg_t) };	// GPIO pin rising edge detect enable 1

		// GPIO pin falling edge detect enable offsets
		constexpr reg_t GPFEN0{ 0x58U / sizeof(reg_t) };	// GPIO pin falling edge detect enable 0
		constexpr reg_t GPFEN1{ 0x5CU / sizeof(reg_t) };	// GPIO pin falling edge detect enable 1

		// GPIO pin high detect enable offsets
		constexpr reg_t GPHEN0{ 0x64U / sizeof(reg_t) };	// GPIO pin high detect enable 0
		constexpr reg_t GPHEN1{ 0x68U / sizeof(reg_t) };	// GPIO pin high detect enable 1

		// GPIO pin low detect enable offsets
		constexpr reg_t GPLEN0{ 0x70U / sizeof(reg_t) };	// GPIO pin low detect enable 1
		constexpr reg_t GPLEN1{ 0x74U / sizeof(reg_t) };	// GPIO pin low detect enable 1

		// GPIO pin async rising edge detect offsets
		constexpr reg_t GPAREN0{ 0x7CU / sizeof(reg_t) };	// GPIO pin async rising edge detect 0
		constexpr reg_t GPAREN1{ 0x80U / sizeof(reg_t) };	// GPIO pin async rising edge detect 1

		// GPIO pin async falling edge detect offsets
		constexpr reg_t GPAFEN0{ 0x88U / sizeof(reg_t) };	// GPIO pin async falling edge detect 0
		constexpr reg_t GPAFEN1{ 0x8CU / sizeof(reg_t) };	// GPIO pin async falling edge detect 1

		// GPIO pull-up/pull-down registers offsets
		constexpr reg_t GPIO_PUP_PDN_CNTRL_REG0{ 0xE4U / sizeof(reg_t) };	// GPIO pull-up/pull-down register 0
		constexpr reg_t GPIO_PUP_PDN_CNTRL_REG1{ 0xE8U / sizeof(reg_t) };	// GPIO pull-up/pull-down register 1
		constexpr reg_t GPIO_PUP_PDN_CNTRL_REG2{ 0xECU / sizeof(reg_t) };	// GPIO pull-up/pull-down register 2
		constexpr reg_t GPIO_PUP_PDN_CNTRL_REG3{ 0xF0U / sizeof(reg_t) };	// GPIO pull-up/pull-down register 3
	}

	// Values for GPFSEL registers.
	enum class __function_select
	{
		gpio_pin_as_input				= 0b000U,
		gpio_pin_as_output				= 0b001U,
		gpio_pin_alternate_function_0	= 0b100U,
		gpio_pin_alternate_function_1	= 0b101U,
		gpio_pin_alternate_function_2	= 0b110U,
		gpio_pin_alternate_function_3	= 0b111U,
		gpio_pin_alternate_function_4	= 0b011U,
		gpio_pin_alternate_function_5	= 0b010U
	};

	// Values for GPFSEL registers.
	enum class pull
	{
		none	= 0b00U,
		up		= 0b01U,
		down	= 0b10U
	};
}