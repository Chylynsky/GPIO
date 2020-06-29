#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace rpi
{
	/*
		Register size in bits.
	*/
	template<typename _Reg>
	constexpr uint32_t reg_size = 8U * sizeof(_Reg);

	/*
		Linux file descriptor RAII wrapper.
	*/
	class __file_descriptor
	{
		const int fd; // File descriptor.

	public:

		// Constructor.
		explicit __file_descriptor(const std::string& path, int flags = 0) : fd{ open(path.c_str(), flags) }
		{
			if (fd < 0)
			{
				throw std::runtime_error("Could not open " + path + ".");
			}
		}

		// Constructor.
		explicit __file_descriptor(std::string&& path, int flags = 0) : fd{ open(path.c_str(), flags) }
		{
			if (fd < 0)
			{
				throw std::runtime_error("Could not open " + path + ".");
			}
		}

		// Constructor
		explicit __file_descriptor(int fd) : fd{ fd } 
		{
		};

		// Destructor.
		~__file_descriptor()
		{
			close(fd);
		}

		operator int()
		{
			return fd;
		}

		// Get file descriptor.
		int get_fd() const noexcept
		{
			return fd;
		}

		__file_descriptor() = delete;
		__file_descriptor& operator=(const __file_descriptor&) = delete;
		__file_descriptor& operator=(__file_descriptor&&) = delete;
	};

	/*
		Functor mapping memory
	*/
	class __Get_reg_ptr
	{
		static volatile reg_t* GPIO_REGISTER_BASE_MAPPED;

		static volatile reg_t* MapMemoryAddressSpace()
		{
			__file_descriptor fd{ "/dev/gpiomem", O_RDWR | O_SYNC };

			volatile void* mapResult = static_cast<volatile void*>(mmap(
				NULL, 
				4096, 
				PROT_READ | PROT_WRITE, 
				MAP_SHARED, 
				fd, 
				0));

			if (mapResult == MAP_FAILED)
			{
				throw std::runtime_error("Unable to map memory.");
			}

			return reinterpret_cast<volatile uint32_t*>(mapResult);
		}

	public:

		volatile uint32_t* operator()(uint32_t reg_offset) const noexcept
		{
			return GPIO_REGISTER_BASE_MAPPED + reg_offset;
		}
	};

	inline volatile uint32_t* __Get_reg_ptr::GPIO_REGISTER_BASE_MAPPED{ __Get_reg_ptr::MapMemoryAddressSpace() };

	/* 
		Global function object. Returns volatile uint32_t pointer
		to the mapped register.
	*/
	static __Get_reg_ptr __get_reg_ptr;
}