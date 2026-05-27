obj-m += my_cdev.o #this line tells the kernels build system we are building a loadable module
# obj-m is a list of object files that get compiled into ko files 

all: module testapp

module: 
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

testapp: test1.c
	gcc test1.c
	
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf a.out