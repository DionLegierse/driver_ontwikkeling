#!/bin/bash

echo 1 > /sys/bus/i2c/drivers/lcd-driver/enable
printf "Hello, World! My name is Dion Legierse and I study Technische Informatica" > /sys/bus/i2c/drivers/lcd-driver/display