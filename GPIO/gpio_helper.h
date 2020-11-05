#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <iostream>
#include <shared_mutex>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace rpi::__impl
{
    /*
        Register size in bits.
    */
    template<typename _Reg>
    inline constexpr _Reg reg_size = 8U * sizeof(_Reg);

    /*
        Linux file descriptor RAII wrapper.
    */
    class file_descriptor
    {
        const int fd; // File descriptor.

    public:

        // Constructor.
        explicit file_descriptor(const std::string& path, int flags = 0) : fd{ open(path.c_str(), flags) }
        {
            if (fd == -1)
            {
                throw std::runtime_error("File " + path + " could not be opened.");
            }
        }

        // Constructor.
        explicit file_descriptor(std::string&& path, int flags = 0) : fd{ open(path.c_str(), flags) }
        {
            if (fd == -1)
            {
                throw std::runtime_error("File " + path + " could not be opened.");
            }
        }

        // Constructor
        explicit file_descriptor(int fd) : fd{ fd } 
        {
            if (fd == -1)
            {
                throw std::runtime_error("Invalid file descriptor.");
            }
        };

        // Destructor.
        ~file_descriptor()
        {
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
            return ::write(fd, buf, size);
        }

        ssize_t read(void* buf, size_t size) const noexcept
        {
            return ::read(fd, buf, size);
        }

        /*
            Deleted functions.
        */
        file_descriptor() = delete;
        file_descriptor& operator=(const file_descriptor&) = delete;
        file_descriptor& operator=(file_descriptor&&) = delete;
    };

    /*
        Functor used to map memory and access it easily.
    */
    template<typename _Reg>
    class Get_reg_ptr
    {
        static volatile _Reg* GPIO_REGISTER_BASE_MAPPED;

        static volatile _Reg* map_memory_address_space()
        {
            const std::string map_file = "/dev/gpiomem";

            std::unique_ptr<file_descriptor> fd;
            
            try
            {
                fd = std::make_unique<file_descriptor>(map_file, O_RDWR | O_SYNC);
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
    volatile _Reg* Get_reg_ptr<_Reg>::GPIO_REGISTER_BASE_MAPPED{ Get_reg_ptr::map_memory_address_space() };

    /* 
        Global function object. Returns volatile uint32_t pointer
        to the mapped register.
    */
    template<typename _Reg>
    inline const Get_reg_ptr<_Reg> get_reg_ptr{};

    /* 
        Get base event register offs.
    */
    template<typename _Reg, typename _Ev>
    inline constexpr _Reg Event_reg_offs = _Ev::offs;
}