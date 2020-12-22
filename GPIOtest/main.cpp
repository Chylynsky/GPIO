#include <thread>
#include <chrono>
#include <iostream>

#include "gpio.h"

using namespace rpi;
using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::this_thread;

int main()
{
    // Declare GPIO pin attached to the LED as output.
    gpio<dir::output> pinLed{ 26U };

    for (int i = 0; i < 10; i++)
    {
        pinLed = HIGH;      // Use optimized assignment operator.
        sleep_for(200ms);
        pinLed = false;     // Use standard assignment operator.
        sleep_for(200ms);
        pinLed.write(1);    // Use function to assign value. Value must be convertible to bool.
        sleep_for(200ms);
        pinLed = 0;
    }

    return 0;
}