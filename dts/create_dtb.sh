#!/bin/bash

cpp -nostdinc -I . -undef -x assembler-with-cpp am335x-boneblack.dts bbb.dts.pre
dtc -I dts -O dtb -o bbb.dtb bbb.dts.pre
