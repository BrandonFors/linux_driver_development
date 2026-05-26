//legacy way of registering a device.
//note that manual creation of device node is necessary here

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BrandonFors - LDD");
MODULE_DESCRIPTION("First character device");

static const char *my_device = "my_cdev";
//major numbers 64-127 are reserved for local or experimental drivers
#define MY_MAJOR 64
//used to store major number if 0 is passed to reguster_chrdev()
// static int major;

static struct file_operations fops = {

};

static int __init my_init(void){
    int status;
    //register device number with the kernel
    // 0 for the major number means assign any unused major num
    // if 0 is used for the major number input, the function returns the assigned major
    // otherwise it returns 
    status = register_chrdev(MY_MAJOR, my_device, &fops);
    if(status < 0){ // negative return of register_chrdev means fail
        pr_err("%s: character device registration failed\n", my_device);
        return status;
    }
    pr_info("%s: Character device registered, Major number: %d\n", my_device, MY_MAJOR);
    return 0;
}

static void __exit my_exit(void){
    unregister_chrdev(MY_MAJOR, my_device);
    pr_info("%s: Character device unregistered, Major number: %d\n", my_device, MY_MAJOR);
}

module_init(my_init);
module_exit(my_exit);