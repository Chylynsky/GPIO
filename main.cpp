#include <iostream>
#include "gpio.h"

int main()
{
    rpi4b::gpio pin(2, rpi4b::GPIODirection::OUTPUT);
    return 0;
}