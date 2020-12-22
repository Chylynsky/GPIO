#include <thread>
#include <chrono>
#include <iostream>

#include "gpio.h"

int main()
{
    using namespace rpi;
    using namespace std;
    using namespace std::chrono;
    using namespace std::chrono_literals;
    using namespace std::this_thread;

    // Declare GPIO pin attached to the LED as output.
    gpio<dir::output> pinLED{ 26U };
    // Declare GPIO pin attached to the button as input
    gpio<dir::input> pinButton{ 26U };
    // Set pull up
    pinButton.set_pull(pull::up);

    for (int i = 0; i < 2; i++)
    {
        pinLED = HIGH;      // Use optimized assignment operator.
        sleep_for(200ms);
        pinLED = false;     // Use standard assignment operator.
        sleep_for(200ms);
        pinLED.write(1);    // Use function to assign value. Value must be convertible to bool.
        sleep_for(200ms);
        pinLED = 0;
        sleep_for(200ms);
    }

    cout << "Press the button!" << endl;

    for (int i = 0; i < 500; i++)
    {
        if (pinButton.read() == 0U)
        {
            pinLED = HIGH;
        }
        else
        {
            pinLED = LOW;
        }

        sleep_for(20ms);
    }

    return 0;
}