cflags-y += -DDEBUG -Wall -Wno-declaration-after-statement
ifneq ($(KERNELRELEASE),)
	obj-m := lcd_spi.o

else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	clear
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	rm -f *.o *.ko *.mod.c modules.order Module.symvers

endif
