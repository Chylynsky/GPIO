#include <iostream>
#include <thread>
#include <chrono>
#include <limits>
#include "gpio.h"

using namespace std::chrono_literals;
using namespace rpi4b;

int main()
{
    GPIO pin(26, GPIODirection::OUTPUT);

    for (int i = 0; i < 10; i++) {
        pin.Write(1U);
        std::this_thread::sleep_for(200ms);
        pin.Write(0U);
        std::this_thread::sleep_for(200ms);
    }

    return 0;
}