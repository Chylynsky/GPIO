#include "gpio.h"

namespace rpi4b
{
	std::multimap<uint32_t, callback_t> __gpio_input_regs::callback_map{};
	std::thread __gpio_input_regs::event_poll_thr{};
	std::mutex __gpio_input_regs::event_poll_mtx{};
}