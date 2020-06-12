// Example blink program

#include <thread>
#include <chrono>
#include <iostream>
#include "gpio.h"

using namespace std;
using namespace std::chrono_literals;
using namespace std::this_thread;
using namespace rpi4b;

int main()
{
    {
        gpio<input> pin(26U);
        pin.set_pull(pull_selection::pull_down);
        sleep_for(200ms);
        //pin.attach_event_callback<pin_high>([]() { cout << "High state detected!" << endl; });
        sleep_for(200ms);
        pin.set_pull(pull_selection::pull_up);
    }


    return 0;
}