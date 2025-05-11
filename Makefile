obj-m += frandom.o
ccflags-y := -std=gnu99 -fcf-protection=full 
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean