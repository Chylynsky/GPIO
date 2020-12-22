# Raspberry Pi 4B GPIO library

This project is aimed to handle simple and efficient access to GPIO on Raspberry Pi 4B platform.

## Usage

Main library project is GPIO. It is written using C++17 and is to be compiled as a static library. If everything
is set correctly, try running an example app in GPIOtest project.

## NOTE

The project is still under development.

## Getting started

The library is created in object oriented way. It also uses compile time parameters in many places in order
to gain performance.

The first thing to do is including gpio.h header file.

```
#include "gpio.h"
```

After that, it is all set up.
Blinking an LED attached to GPIO pin is as simple as:

```
gpio<dir::output> pinLED{ 26 };

pinLED = 1;
pinLED = 0;
```

Class *gpio* is the main class used for GPIO operations. Passing template parameter from *dir* (direction) namespace, which is either
*input* or *output*, defines the way the class behaves. When the gpio object is created as output, all the output methods became available to user.
These methods are mainly assignment operators. While constructing, the number of GPIO pin needs to be passed to the constructor.
Assigning a value to the output can be performed in, basically, three ways:
- by assigning a value convertible to bool (int, turnary operator etc.),
- by using optimized assignment,
- by using *write* method.

The first option is depicted in the example above. It's for the best to decide on one type to use here. If you choose bool for representing states and stick
with it, your program will end up producing less machine code.
The second option uses *HIGH* and *LOW* compile time constants. This is the most efficient way of assigning the value to the GPIO pin. It produces
the same number of assembly instructions as a minimal C code able to perform this task.

```
gpio<dir::output> pinLED{ 26 };

pinLED = HIGH;
pinLED = LOW;
```

The third, and last, option uses *write* method. The rule here is the same as with first option - it is best to stick to one type.

```
gpio<dir::output> pinLED{ 26 };

pinLED.write(true);
pinLED.write(false);
```

When a *gpio* object is created with *dir::input* template parameter, the input methods became available, which is *read*, *set_pull* and *attach_irq_callback* (experimental, read below).

```
gpio<dir::input> pinBtn{ 26 };
pinBtn.set_pull(pull::up);

int value = pinBtn.read();
```

It is a good practice to set a desired pull via *set_pull* method as soon as possible. The three possible arguments here are hold in *pull* enum class. Its values are
*pull::none*, *pull::up* and *pull::down*.
When the desired pull resistor is set, the pin is ready to perform the read. The *read* method returns an integer 1 when the current pin state is high and 0 if it is low.

Every resource is released and put back to its original state when *gpio* object reaches the end of its scope.

## EXPERIMENTAL

If you #define an EXPERIMENTAL preprocessor macro you get access to the experimental functions of the library. Theese functions are under development so may not behave as expected.
When dealing with GPIO pins we eventually find ourselves in need of interrupt handlers. The *GPIOdriver* part of the project allows that. It is a kernel module passing events that the user
had requested to the userspace. In order to mount the module, use *gpiodev_init.sh* script.
With the module mounted and EXPERIMENTAL defined we can create our first event handler:

```
gpio<dir::input> pinBtn{ 26 };
pinBtn.set_pull(pull::down);

pinBtn.attach_irq_callback<irq::rising_edge>([](){ std::cout << "Interrupt handled!"; });
```

Inside of the *irq* namespace one can find all the handled events. The event chosen is then passed to *attach_irq_callback* method as a template parameter. The argument passed to the method
can be a function pointer, lambda or std::function<void()>, bearing in mind it has to be *void fun()* type of the function.

## Author
* **Borys Chyliński** - *Backend* - [Chylynsky](https://github.com/Chylynsky)