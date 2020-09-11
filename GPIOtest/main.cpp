#include <thread>
#include <chrono>
#include <iostream>
#include <bitset>

#define BCM2711
#include "gpio.h"

using namespace rpi;
using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;
using namespace std::this_thread;

int main()
{
    constexpr auto PROGRAM_WAIT_TIME{ 60s };
    constexpr uint32_t LED_PIN_NUMBER{ 26U };
    constexpr uint32_t BTN_PIN_NUMBER{ 25U };

    gpio<dir::output> pinLED(LED_PIN_NUMBER);	// GPIO pin with LED attached

    // Create callback lambda that makes an LED blink twice
    auto blink = [&pinLED]()
    {
        // First blink uses assignment operator
        pinLED = 1;
        sleep_for(100ms);
        pinLED = 0;
        sleep_for(100ms);

        // Second blink uses write function
        pinLED.write(1);
        sleep_for(100ms);
        pinLED.write(0);
        sleep_for(100ms);

        cout << "Blink!" << endl;
    };

    gpio<dir::input> pinBtn(BTN_PIN_NUMBER);	// GPIO pin with button attached

    pinBtn.set_pull(pull::up);	// Set pull up resistor

    // Call "blink" lambda when signal is pulled down by the button
    pinBtn.attach_irq_callback<irq::falling_edge>(blink);

    cout << "Push the button attached to pin " << std::to_string(BTN_PIN_NUMBER) << " and enjoy the blinking LED!" << endl;
    cout << "The program will exit after " << PROGRAM_WAIT_TIME.count() << " seconds." << endl;

    sleep_for(PROGRAM_WAIT_TIME);

    return 0;
}