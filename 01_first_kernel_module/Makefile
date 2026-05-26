obj-m += hello.o #this line tells the kernels build system we are building a loadable module
# obj-m is a list of object files that get compiled into ko files 

all: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean