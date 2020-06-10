#include <iostream>
#include <thread>
#include <chrono>
#include <limits>
#include "gpio.h"

using namespace std::chrono_literals;
using namespace rpi4b;

int main()
{
    gpio<output> pin(26U);

    for (int i = 0; i < 10; i++) {
        pin.write(1U);
        std::this_thread::sleep_for(200ms);
        pin.write(0U);
        std::this_thread::sleep_for(200ms);
    }

    return 0;
}