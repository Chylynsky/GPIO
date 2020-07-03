#pragma once
#include <functional>
#include "gpio_predicates.h"

namespace rpi
{
	using callback_t = std::function<void()>;
}