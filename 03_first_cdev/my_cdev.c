#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BrandonFors - LDD");
MODULE_DESCRIPTION("First character device");

static const char *my_device = "my_cdev";

//holds allocated device number, major and minor
static dev_t dev_nr;

static struct cdev my_cdev;

static struct class *my_class;

static struct file_operations fops = {

};

static int __init my_init(void){
    int status;
    //modern way of registering a device
    //allocate device number region
    status = alloc_chrdev_region(&dev_nr, 0, MINORMASK + 1, my_device);
    if(status < 0){ // negative return of register_chrdev means fail
        pr_err("%s: character device registration failed\n", my_device);
        return status;
    }
    //initialize file operations and dev structure
    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;
    //add device to kernel device list
    status = cdev_add(&my_cdev, dev_nr, MINORMASK + 1);
    if(status){
        goto free_device_nr;
    }

    // create a struct class pointer
    my_class = class_create("my_class");
    if(!my_class){
        pr_err("%s, could not create class my_class\n", my_device);
        status = ENOMEM;
        goto delete_cdev;
    }

    //creates node using subsystem called udev
    //create node in userspace
    if(!device_create(my_class, NULL, dev_nr, NULL, "my_cdev%d", 0)){
        pr_err("%s: Could not create device my_cdev0\n", my_device);
        status = ENOMEM;
        goto delete_class;
    }

    pr_info("%s: Character device registered, Major number: %d, Minor number: %d\n", my_device, MAJOR(dev_nr), MINOR(dev_nr));
    pr_info("%s: Created device number under /sys/class/my_class\n", my_device);
    pr_info("%s: Created new device node /dev/my_cdev0\n", my_device);
    return 0;

delete_class:
    //free up class structure
    class_destroy(my_class);

delete_cdev:
    //delete the device
    cdev_del(&my_cdev);
free_device_nr:
    //unregister character device region
    unregister_chrdev_region(dev_nr, MINORMASK + 1);
    return status;
}

static void __exit my_exit(void){
    device_destroy(my_class, dev_nr);
    //free up class structure
    class_destroy(my_class);
    //delete the device
    cdev_del(&my_cdev);
    //unregister character device region
    unregister_chrdev_region(dev_nr, MINORMASK + 1);
    pr_info("%s: Character device unregistered, Major number: %d, Minor number: %d\n", my_device, MAJOR(dev_nr), MINOR(dev_nr));
}

module_init(my_init);
module_exit(my_exit);