#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <iostream>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace rpi
{
	/*
		Register size in bits.
	*/
	template<typename _Reg>
	inline constexpr _Reg reg_size = 8U * sizeof(_Reg);

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

		// Implicit conversion to int.
		operator int()
		{
			return fd;
		}

		// Get file descriptor.
		int get_fd() const noexcept
		{
			return fd;
		}

		/*
			Deleted functions.
		*/
		__file_descriptor() = delete;
		__file_descriptor& operator=(const __file_descriptor&) = delete;
		__file_descriptor& operator=(__file_descriptor&&) = delete;
	};

	/*
		Functor used to map memory and access it easily.
	*/
	template<typename _Reg>
	class __Get_reg_ptr
	{
		static volatile _Reg* GPIO_REGISTER_BASE_MAPPED;

		static volatile _Reg* MapMemoryAddressSpace()
		{
			std::unique_ptr<__file_descriptor> fd;

			try {
				fd = std::make_unique<__file_descriptor>("/dev/gpiomem", O_RDWR | O_SYNC);
			}
			catch (const std::runtime_error& e) {
				throw e; // No '/dev/gpiomem' file
			}

			volatile void* mapResult = 
				static_cast<volatile void*>(mmap(
					NULL, 
					4096, 
					PROT_READ | PROT_WRITE, 
					MAP_SHARED, 
					*fd,
					0));

			if (mapResult == MAP_FAILED)
			{
				throw std::runtime_error("Unable to map memory.");
			}

			return reinterpret_cast<volatile _Reg*>(mapResult);
		}

	public:

		volatile _Reg* operator()(_Reg reg_offset) const noexcept
		{
			return GPIO_REGISTER_BASE_MAPPED + reg_offset;
		}
	};

	template<typename _Reg>
	inline volatile _Reg* __Get_reg_ptr<_Reg>::GPIO_REGISTER_BASE_MAPPED{ __Get_reg_ptr::MapMemoryAddressSpace() };

	/* 
		Global function object. Returns volatile uint32_t pointer
		to the mapped register.
	*/
	template<typename _Reg>
	inline const __Get_reg_ptr<_Reg> __get_reg_ptr{};

	// Get base event register offs.
	template<typename _Reg, typename _Ev>
	inline constexpr _Reg __Event_reg_offs = _Ev::offs;
}