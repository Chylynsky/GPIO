#pragma once
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <memory>

#include "gpio_direction.h"
#include "gpio_traits.h"
#include "gpio_events.h"
#include "gpio_input.h"
#include "gpio_output.h"
#include "gpio_aliases.h"
#include "gpio_helper.h"

#include "bcm2711.h"

namespace rpi
{
    inline constexpr std::true_type  HIGH{};
    inline constexpr std::false_type LOW{};

    /*
        Template class gpio allows access to gpio with direction
        specified by the template type _Dir and register type reg_t.
    */
    template<typename _Dir>
    class gpio : private __impl::traits::Derive_if<__impl::traits::Is_input<_Dir>, __impl::gpio_input<reg_t>, __impl::gpio_output<reg_t>>
    {
        static_assert(__impl::traits::Is_direction<_Dir>, "Template type _Dir must be either dir::input or dir::output.");
        static_assert(__impl::traits::Is_intergral<reg_t>, "Template type reg_t must be integral.");

        // Select FSEL register offset based on GPIO pin number.
        volatile reg_t* get_function_select_reg() const noexcept;

        const reg_t reg_bit_set_val; // Value OR'ed with registers responsible for GPIO state (GPSET, GPCLR, ...)
        const uint32_t pin_number;  // GPIO pin number.

    public:

        explicit gpio(uint32_t pin_number);
        ~gpio();

        // Output methods
        
        template<typename _Arg, typename _Ty = _Dir>
        __impl::traits::Enable_if<
            __impl::traits::Is_output<_Ty> && !__impl::traits::Is_constant<_Arg>, void> operator=(_Arg state) noexcept;
        
        template<typename _Arg, typename _Ty = _Dir>
        __impl::traits::Enable_if<
            __impl::traits::Is_output<_Ty> && __impl::traits::Is_constant<_Arg>, void> operator=(_Arg) noexcept;

        // Write to GPIO pin
        template<typename _Arg = int, typename _Ty = _Dir>
        __impl::traits::Enable_if<
            __impl::traits::Is_output<_Ty>, void> write(_Arg state) noexcept;

        // Input methods.

        // Read current GPIO pin state.
        template<typename _Ty = _Dir>
        __impl::traits::Enable_if<
            __impl::traits::Is_input<_Ty>, uint32_t> read() const noexcept;

        // Set pull-up, pull-down or none.
        template<typename _Ty = _Dir>
        __impl::traits::Enable_if<
            __impl::traits::Is_input<_Ty>, void> set_pull(pull pull_sel) noexcept;

        // Get current pull type.
        template<typename _Ty = _Dir>
        __impl::traits::Enable_if<
            __impl::traits::Is_input<_Ty>, pull> get_pull() noexcept;

#ifdef EXPERIMENTAL

        // Set event callback.
        template<typename _Ev, typename _Ty = _Dir>
        __impl::traits::Enable_if <
            __impl::traits::Is_input<_Ty> && __impl::traits::Is_event<_Ev>, void> attach_irq_callback(const callback_t& callback);

#endif

        // Deleted methods.

        gpio() = delete;
        gpio(const gpio&) = delete;
        gpio(gpio&&) = delete;
        gpio& operator=(const gpio&) = delete;
        gpio& operator=(gpio&&) = delete;
    };

    template<typename _Dir>
    inline volatile reg_t* gpio<_Dir>::get_function_select_reg() const noexcept
    {
        /* 
            Each pin function is described by 3 bits,
            each register controls 10 pins
        */
        return __impl::get_reg_ptr<reg_t>(__impl::addr::GPFSEL0 + pin_number / 10U);
    }

    template<typename _Dir>
    gpio<_Dir>::gpio(uint32_t pin_number) : reg_bit_set_val{ 1U << (pin_number % __impl::reg_size<reg_t>) }, pin_number{ pin_number }
    {
        // Select GPIO function select register.
        volatile reg_t* function_select_reg = get_function_select_reg();

        // Each pin represented by three bits.
        reg_t fsel_bit_shift = (3U * (pin_number % 10U));

        // Clear function select register bits.
        *function_select_reg &= ~(0b111U << fsel_bit_shift);

        uint32_t reg_index = pin_number / __impl::reg_size<reg_t>;

        // Enable only when instantiated with input template parameter.
        if constexpr (__impl::traits::Is_input<_Dir>)
        {
            *function_select_reg |= static_cast<uint32_t>(__impl::function_select::gpio_pin_as_input) << fsel_bit_shift;
            __impl::gpio_input<reg_t>::level_reg = __impl::get_reg_ptr<reg_t>(__impl::addr::GPLEV0 + reg_index);
        }
        
        // Enable only when instantiated with output template parameter.
        if constexpr (__impl::traits::Is_output<_Dir>)
        {
            *function_select_reg |= static_cast<uint32_t>(__impl::function_select::gpio_pin_as_output) << fsel_bit_shift;

            // 31 pins are described by the first GPSET and GPCLR registers.
            __impl::gpio_output<reg_t>::set_reg = __impl::get_reg_ptr<reg_t>(__impl::addr::GPSET0 + reg_index);
            __impl::gpio_output<reg_t>::clr_reg = __impl::get_reg_ptr<reg_t>(__impl::addr::GPCLR0 + reg_index);
        }
    }

    template<typename _Dir>
    gpio<_Dir>::~gpio()
    {
        // Enable only when instantiated with input template parameter.
        if constexpr (__impl::traits::Is_input<_Dir>)
        {
            reg_t reg_bit_clr_val = ~reg_bit_set_val;

            // Clear event detect bits.
            for (volatile reg_t* reg : __impl::gpio_input<reg_t>::event_regs_used)
            {
                *reg &= reg_bit_clr_val;
                __impl::gpio_input<reg_t>::irqs_set--;
            }

            __impl::gpio_input<reg_t>::irq_controller->irq_free(pin_number);

            if (__impl::gpio_input<reg_t>::irqs_set == 0U)
            {
                __impl::gpio_input<reg_t>::irq_controller.reset();
            }

            // Set pull-down resistor.
            set_pull(pull::down);
        }

        // Enable only when instantiated with output template parameter.
        if constexpr (__impl::traits::Is_output<_Dir>)
        {
            *__impl::gpio_output<reg_t>::clr_reg |= reg_bit_set_val;
        }

        // Reset function select register.
        *(get_function_select_reg()) &= ~(0b111U << (3U * (pin_number % 10U)));
    }
    
    template<typename _Dir>
    template<typename _Arg, typename _Ty>
    __impl::traits::Enable_if<
        __impl::traits::Is_output<_Ty> && !__impl::traits::Is_constant<_Arg>, void> gpio<_Dir>::operator=(_Arg state) noexcept
    {
        // Set '1' in SET or CLR register.
        if (!state)
        {
            *__impl::gpio_output<reg_t>::clr_reg |= reg_bit_set_val;
        }
        else
        {
            *__impl::gpio_output<reg_t>::set_reg |= reg_bit_set_val;
        }
    }
    
    template<typename _Dir>
    template<typename _Arg, typename _Ty>
    inline __impl::traits::Enable_if<
        __impl::traits::Is_output<_Ty> && __impl::traits::Is_constant<_Arg>, void> gpio<_Dir>::operator=(_Arg) noexcept
    {
        // Set '1' in SET or CLR register.
        if constexpr (!_Arg::value)
        {
            *__impl::gpio_output<reg_t>::clr_reg |= reg_bit_set_val;
        }
        else
        {
            *__impl::gpio_output<reg_t>::set_reg |= reg_bit_set_val;
        }
    }

    template<typename _Dir>
    template<typename _Arg, typename _Ty>
    inline __impl::traits::Enable_if<
        __impl::traits::Is_output<_Ty>, void> gpio<_Dir>::write(_Arg state) noexcept
    {
        *this = state;
    }

    template<typename _Dir>
    template<typename _Ty>
    inline __impl::traits::Enable_if<
        __impl::traits::Is_input<_Ty>, uint32_t> gpio<_Dir>::read() const noexcept
    {
        return (*__impl::gpio_input<reg_t>::level_reg >> (pin_number % __impl::reg_size<reg_t>)) & 1U;
    }

    template<typename _Dir>
    template<typename _Ty>
    __impl::traits::Enable_if<
        __impl::traits::Is_input<_Ty>, void> gpio<_Dir>::set_pull(pull pull_sel) noexcept
    {
        // 16 pins are controlled by each register.
        volatile reg_t* reg_sel = __impl::get_reg_ptr<reg_t>(__impl::addr::GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));

         // Each pin is represented by two bits, 16 pins described by each register.
        reg_t bit_shift = 2U * (pin_number % 16U);

        // Clear the bit and set it.
        *reg_sel &= ~(0b11U << bit_shift);
        *reg_sel |= static_cast<reg_t>(pull_sel) << bit_shift;
    }

    template<typename _Dir>
    template<typename _Ty>
    __impl::traits::Enable_if<
        __impl::traits::Is_input<_Ty>, pull> gpio<_Dir>::get_pull() noexcept
    {
        // 16 pins are controlled by each register.
        volatile reg_t* reg_sel = __impl::get_reg_ptr<reg_t>(__impl::addr::GPIO_PUP_PDN_CNTRL_REG0 + (pin_number / 16U));
        return static_cast<pull>(*reg_sel >> (2U * (pin_number % 16U)) & 0b11U);
    }

#ifdef EXPERIMENTAL

    template<typename _Dir>
    template<typename _Ev, typename _Ty>
    __impl::traits::Enable_if<
        __impl::traits::Is_input<_Ty> && __impl::traits::Is_event<_Ev>,
        void> gpio<_Dir>::attach_irq_callback(const callback_t& callback)
    {
        // Get event register based on event type.
        volatile reg_t* event_reg = __impl::get_reg_ptr<reg_t>(__impl::Event_reg_offs<reg_t, _Ev> + pin_number / __impl::reg_size<reg_t>);

        if (__impl::gpio_input<reg_t>::irqs_set == 0U)
        {
            try
            {
                __impl::gpio_input<reg_t>::irq_controller = std::make_unique<__impl::irq_controller>();
            }
            catch (const std::runtime_error& err)
            {
                throw err;
            }
        }

        try
        {
            __impl::gpio_input<reg_t>::irq_controller->request_irq(pin_number, callback);
        }
        catch (const std::runtime_error& err)
        {
            throw err;
        }

        __impl::gpio_input<reg_t>::irqs_set++;

        // Clear then set bit responsible for the selected pin.
        *event_reg &= ~reg_bit_set_val;
        *event_reg |= reg_bit_set_val;

        __impl::gpio_input<reg_t>::event_regs_used.push_back(event_reg);
    }

#endif
}