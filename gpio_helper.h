#pragma once
#include <cstdint>
#include <stdexcept>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace rpi4b
{
	template<typename _Reg>
	constexpr _Reg reg_size = 8 * sizeof(_Reg);

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

	// Global function object. Returns volatile uint32_t pointer
	// to the mapped register.
	constexpr __get_reg_ptr get_reg_ptr;
}