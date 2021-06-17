#!/bin/bash

echo 1 > /sys/bus/i2c/drivers/lcd-driver/enable
printf "Dit is een test string voor mijn lcd driver" > /sys/bus/i2c/drivers/lcd-driver/display