#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <iostream>
#include <mutex>

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
		mutable std::mutex mtx;
		const int fd; // File descriptor.

	public:

		// Constructor.
		explicit __file_descriptor(const std::string& path, int flags = 0) : fd{ open(path.c_str(), flags) }
		{
			if (fd == -1)
			{
				throw std::runtime_error("File " + path + " could not be opened.");
			}
		}

		// Constructor.
		explicit __file_descriptor(std::string&& path, int flags = 0) : fd{ open(path.c_str(), flags) }
		{
			if (fd == -1)
			{
				throw std::runtime_error("File " + path + " could not be opened.");
			}
		}

		// Constructor
		explicit __file_descriptor(int fd) : fd{ fd } 
		{
			if (fd == -1)
			{
				throw std::runtime_error("Invalid file descriptor.");
			}
		};

		// Destructor.
		~__file_descriptor()
		{
			std::unique_lock<std::mutex> lock{ mtx };
			close(fd);
		}

		// Implicit conversion to int.
		operator int() const noexcept
		{
			return fd;
		}

		// Get file descriptor.
		int get_fd() const noexcept
		{
			return fd;
		}

		ssize_t write(const void* buf, size_t size) const noexcept
		{
			std::unique_lock<std::mutex> lock{ mtx };
			return ::write(fd, buf, size);
		}

		ssize_t read(void* buf, size_t size) const noexcept
		{
			std::unique_lock<std::mutex> lock{ mtx };
			return ::read(fd, buf, size);
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
			const std::string map_file = "/dev/gpiomem";

			std::unique_ptr<__file_descriptor> fd;
			
			try
			{
				fd = std::make_unique<__file_descriptor>(map_file, O_RDWR | O_SYNC);
			}
			catch (const std::runtime_error& err)
			{
				throw err;
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
	volatile _Reg* __Get_reg_ptr<_Reg>::GPIO_REGISTER_BASE_MAPPED{ __Get_reg_ptr::MapMemoryAddressSpace() };

	/* 
		Global function object. Returns volatile uint32_t pointer
		to the mapped register.
	*/
	template<typename _Reg>
	inline const __Get_reg_ptr<_Reg> __get_reg_ptr{};

	/* 
		Get base event register offs.
	*/
	template<typename _Reg, typename _Ev>
	inline constexpr _Reg __Event_reg_offs = _Ev::offs;
}