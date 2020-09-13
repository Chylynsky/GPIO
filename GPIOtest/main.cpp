#include <thread>
#include <chrono>
#include <iostream>
#include <bitset>
#include <sstream>
#include <fstream>

#define BCM2711
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

    // Create callback function that gets called when the button is pushed.
    auto blink = [&pinLed]() 
    {
        // Use predefined HIGH and LOW states for optimized assignment operator. 
        pinLed = HIGH;
        sleep_for(100ms);
        pinLed = LOW;
        sleep_for(100ms);

        /* 
        * Use any type that you like for high and low state representation,
        * but it is best to decide on one.
        */
        pinLed = 1;
        sleep_for(100ms);
        pinLed = false;
        sleep_for(100ms);

        /*
        * Another approach is to use a function. The flexibility of type
        * choice remains the same.
        */
        pinLed.write(1);
        sleep_for(100ms);
        pinLed.write(0);
        sleep_for(100ms);
    };

    // Declare GPIO pin attached to button as input.
    gpio<dir::input> pinButton{ 25U };

    // Set pull-up resistor.
    pinButton.set_pull(pull::up);

    /*
    * Attach lambda created earlier as a callback function for falling edge event
    * on GPIO pin attached to the button.
    */
    pinButton.attach_irq_callback<irq::falling_edge>(blink);

    // Exit after 60s.
    sleep_for(60s);

    return 0;
}