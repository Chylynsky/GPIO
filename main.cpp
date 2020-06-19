// Example blink program

#include <thread>
#include <chrono>
#include <iostream>
#include <future>
#include "gpio.h"

using namespace std;
using namespace std::chrono_literals;
using namespace std::this_thread;
using namespace rpi4b;

int main()
{
	gpio<output> pin(26);
	
	pin.write(1);

    return 0;
}