obj-m += GPIOdriver.o
all:
    make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
    -sudo rmmod GPIOdriver
    make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean