// Example blink program

#include <thread>
#include <chrono>
#include <iostream>
#include "gpio.h"
#include "dispatch_queue.h"

using namespace std;
using namespace std::chrono_literals;
using namespace std::this_thread;
using namespace rpi4b;

int main()
{
    auto x = []() { std::cout << "1!"; };
    auto y = []() { std::cout << "2!"; };
    auto z = []() { std::cout << "3!"; };

    dispatch_queue<std::function<void(void)>> cq;

    sleep_for(200ms);

    cq.push(x);
    cq.push(y);
    cq.push(z);

    sleep_for(200ms);

    cq.push(x);
    cq.push(y);
    cq.push(z);

    sleep_for(200ms);

    //gpio<output> pin(26U);
    //pin.write(1);

    return 0;
}