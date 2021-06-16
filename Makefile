KDIR = /lib/modules/$(shell uname -r)/build
PWD = $(shell pwd)
MAKE = /usr/bin/make
CPPFLAGS:=-std=c11 -W -Wall -pedantic -Werror

%.ko : %.c
	$(MAKE) $(*).ko obj-m=$(*).o -C $(KDIR) M=$(PWD)  modules 
    
clean:
	rm -f *.mod.o
	rm -f *.mod.c
	rm -f *.mod
	rm -f *.o
	rm -f *.order
	rm -f *.symvers
	rm -f .*.cmd
	rm -f .*.ko.cmd
	rm -f .*.mod.cmd
	rm -f .*.mod.o.cmd
	rm -f .*.o.cmd
	rm -f .*.symvers.cmd
	rm -f .*.o.d

clean-all:
	rm -f *.ko
	make clean
