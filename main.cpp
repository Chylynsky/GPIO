// Example blink program

#include <thread>
#include <chrono>
#include <iostream>
#include "gpio.h"

using namespace std::chrono_literals;
using namespace rpi4b;

int main()
{
    gpio<input> pin(23U);
    pin.set_pull(gpio_pup_pdn_cntrl_value::pull_up);

    std::cout << (uint32_t)pin.get_pull() << std::endl;

    return 0;
}