#pragma once
#include <functional>
#include "gpio_traits.h"

namespace rpi
{
    using callback_t = std::function<void()>;
}