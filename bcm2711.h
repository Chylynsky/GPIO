#pragma once
#include <cstdint>
#include <stdexcept>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// BCM2711 documentation:
// https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2711/rpi_DATA_2711_1p0.pdf

namespace rpi4b
{
	// GPIO function select offsets
	inline constexpr uint32_t GPFSEL0	{ 0x00U / sizeof(uint32_t) };	// GPIO function select 0
	inline constexpr uint32_t GPFSEL1	{ 0x04U / sizeof(uint32_t) };	// GPIO function select 1
	inline constexpr uint32_t GPFSEL2	{ 0x08U / sizeof(uint32_t) };	// GPIO function select 2
	inline constexpr uint32_t GPFSEL3	{ 0x0CU / sizeof(uint32_t) };	// GPIO function select 3
	inline constexpr uint32_t GPFSEL4	{ 0x10U / sizeof(uint32_t) };	// GPIO function select 4
	inline constexpr uint32_t GPFSEL5	{ 0x14U / sizeof(uint32_t) };	// GPIO function select 5

	// Number of bits that every pin occupies in gpfsel registers
	inline constexpr uint32_t GPFSEL_PIN_BIT_SIZE	{ 3 };

	// Values for GPFSEL registers
	enum class GPIOFunctionSelect
	{
		GPIO_PIN_AS_INPUT				= 0b000U,
		GPIO_PIN_AS_OUTPUT				= 0b001U,
		GPIO_PIN_ALTERNATE_FUNCTION_0	= 0b100U,
		GPIO_PIN_ALTERNATE_FUNCTION_1	= 0b101U,
		GPIO_PIN_ALTERNATE_FUNCTION_2	= 0b110U,
		GPIO_PIN_ALTERNATE_FUNCTION_3	= 0b111U,
		GPIO_PIN_ALTERNATE_FUNCTION_4	= 0b011U,
		GPIO_PIN_ALTERNATE_FUNCTION_5	= 0b010U
	};

	// GPIO pin output set offsets
	inline constexpr uint32_t GPSET0	{ 0x1CU / sizeof(uint32_t) };	// GPIO pin output set 0
	inline constexpr uint32_t GPSET1	{ 0x20U / sizeof(uint32_t) };	// GPIO pin output set 1

	// GPIO pin output clear offsets
	inline constexpr uint32_t GPCLR0	{ 0x28U / sizeof(uint32_t) };	// GPIO pin output clear 0
	inline constexpr uint32_t GPCLR1	{ 0x2CU / sizeof(uint32_t) };	// GPIO pin output clear 1

	// GPIO pin level offsets
	inline constexpr uint32_t GPLEV0	{ 0x34U / sizeof(uint32_t) };	// GPIO pin level 0
	inline constexpr uint32_t GPLEV1	{ 0x38U / sizeof(uint32_t) };	// GPIO pin level 1

	// GPIO pin event detect status offsets
	inline constexpr uint32_t GPEDS0	{ 0x40U / sizeof(uint32_t) };	// GPIO pin event detect status 0
	inline constexpr uint32_t GPEDS1	{ 0x44U / sizeof(uint32_t) };	// GPIO pin event detect status 1

	// GPIO pin rising edge detect enable offsets
	inline constexpr uint32_t GPREN0	{ 0x4CU / sizeof(uint32_t) };	// GPIO pin rising edge detect enable 0
	inline constexpr uint32_t GPREN1	{ 0x50U / sizeof(uint32_t) };	// GPIO pin rising edge detect enable 1

	// GPIO pin falling edge detect enable offsets
	inline constexpr uint32_t GPFEN0	{ 0x58U / sizeof(uint32_t) };	// GPIO pin falling edge detect enable 0
	inline constexpr uint32_t GPFEN1	{ 0x5CU / sizeof(uint32_t) };	// GPIO pin falling edge detect enable 1

	// GPIO pin high detect enable offsets
	inline constexpr uint32_t GPHEN0	{ 0x64U / sizeof(uint32_t) };	// GPIO pin high detect enable 0
	inline constexpr uint32_t GPHEN1	{ 0x68U / sizeof(uint32_t) };	// GPIO pin high detect enable 1

	// GPIO pin low detect enable offsets
	inline constexpr uint32_t GPLEN0	{ 0x70U / sizeof(uint32_t) };	// GPIO pin low detect enable 1
	inline constexpr uint32_t GPLEN1	{ 0x74U / sizeof(uint32_t) };	// GPIO pin low detect enable 1

	// GPIO pin async rising edge detect offsets
	inline constexpr uint32_t GPAREN0	{ 0x7CU / sizeof(uint32_t) };	// GPIO pin async rising edge detect 0
	inline constexpr uint32_t GPAREN1	{ 0x80U / sizeof(uint32_t) };	// GPIO pin async rising edge detect 1

	// GPIO pin async falling edge detect offsets
	inline constexpr uint32_t GPAFEN0	{ 0x88U / sizeof(uint32_t) };	// GPIO pin async falling edge detect 0
	inline constexpr uint32_t GPAFEN1	{ 0x8CU / sizeof(uint32_t) };	// GPIO pin async falling edge detect 1

	// GPIO pull-up/pull-down registers offsets
	inline constexpr uint32_t GPIO_PUP_PDN_CNTRL_REG0	{ 0xE4U / sizeof(uint32_t) };	// GPIO pull-up/pull-down register 0
	inline constexpr uint32_t GPIO_PUP_PDN_CNTRL_REG1	{ 0xE8U / sizeof(uint32_t) };	// GPIO pull-up/pull-down register 1
	inline constexpr uint32_t GPIO_PUP_PDN_CNTRL_REG2	{ 0xECU / sizeof(uint32_t) };	// GPIO pull-up/pull-down register 2
	inline constexpr uint32_t GPIO_PUP_PDN_CNTRL_REG3	{ 0xF0U / sizeof(uint32_t) };	// GPIO pull-up/pull-down register 3

	// Values for GPFSEL registers
	enum class ResistorSelect
	{
		NO_PULL		= 0b00,
		PULL_UP		= 0b01,
		PULL_DOWN	= 0b10
	};

	class __get_reg_ptr
	{
		static volatile uint32_t* GPIO_REGISTER_BASE_MAPPED;

		static volatile uint32_t* MapMemoryAddressSpace()
		{
			int gpiomem = open("/dev/gpiomem", O_RDWR | O_SYNC);

			if (gpiomem < 0) 
			{
				throw std::runtime_error("Unable to open /dev/gpiomem.");
			}

			volatile void* mapResult = mmap(nullptr, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, gpiomem, 0);
			close(gpiomem);

			if (mapResult == MAP_FAILED) 
			{
				throw std::runtime_error("Unable to map memory.");
			}
			else 
			{
				return reinterpret_cast<volatile uint32_t*>(mapResult);
			}
		}

	public:

		volatile uint32_t* operator()(uint32_t reg_offset) const noexcept
		{
			return GPIO_REGISTER_BASE_MAPPED + reg_offset;
		}
	};

	inline volatile uint32_t* __get_reg_ptr::GPIO_REGISTER_BASE_MAPPED{ __get_reg_ptr::MapMemoryAddressSpace() };

	inline constexpr __get_reg_ptr get_reg_ptr;
}