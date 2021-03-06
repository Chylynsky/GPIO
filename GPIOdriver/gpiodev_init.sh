#!/bin/sh

module="GPIOdriver"
device="gpiodev"

# remove old version and load the new one
/sbin/rmmod $module
/sbin/insmod ./$module.ko

# remove old device nodes
rm -f /dev/$device

# find the major number of the device
major=$(grep $device /proc/devices | cut -d ' ' -f 1)
mknod /dev/$device c $major 0

# give read/write permission to everybody
chmod a=rw /dev/gpiodev
