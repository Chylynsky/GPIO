// Example blink program

#include <thread>
#include <chrono>
#include "gpio.h"

using namespace std::chrono_literals;
using namespace rpi4b;

int main()
{
    gpio<output> pin(26U);

    for (int i = 0; i < 50; i++) {
        pin.write(true);
        std::this_thread::sleep_for(200ms);
        pin.write(false);
        std::this_thread::sleep_for(200ms);
    }

    return 0;
}