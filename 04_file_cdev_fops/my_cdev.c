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

#define DEV_BUFFER_SIZE 64
//memory variables
static char *dev_buffer;
static struct mutex dev_mutex;

static int my_open(struct inode *pInode, struct file *pFile){
    pr_info("%s: my_open called, Major: %d, Minor: %d\n", my_device, imajor(pInode), iminor(pInode));

    pr_info("%s: file->f_pos: %lld\n", my_device, pFile->f_pos);
    pr_info("%s: file->f_mode: 0x%x\n", my_device, pFile->f_mode);
    pr_info("%s: file->f_flags: 0x%x\n", my_device, pFile->f_flags);
    return 0;
}
static int my_release(struct inode *pInode, struct file *pFile){
    pr_info("%s: my_release called, file is closed\n", my_device);

    return 0;
}

static ssize_t my_read(struct file *pFile, char __user *pUser_buff, size_t count, loff_t *pOffset){
    size_t bytes_to_copy, not_copied, copied;
    
    //aquire the mutex and tell the kernel that the syscall to restart the syscall if needed when the mutex is taken already
    if(mutex_lock_interruptible(&dev_mutex)) return -ERESTARTSYS;

    pr_info("%s: read called: request=%zu, offset=%lld\n", my_device, count, *pOffset);

    bytes_to_copy = (count + *pOffset > strlen(dev_buffer)) ? (strlen(dev_buffer) - *pOffset) : count;

    pr_info("%s: Read will copy %zu bytes\n", my_device, bytes_to_copy);

    //returns number of bytes that could not be copied
    not_copied = copy_to_user(pUser_buff, dev_buffer + *pOffset, bytes_to_copy);
    copied = bytes_to_copy - not_copied;

    *pOffset += copied;
    if(not_copied) pr_warn("%s: copy_to_user only copied %zu/%zu\n", my_device, copied, bytes_to_copy);

    pr_info("%s: Read done: return=%zu, new offset=%lld\n", my_device, copied, *pOffset);

    mutex_unlock(&dev_mutex);
    return (ssize_t) copied;
}
static ssize_t my_write(struct file *pFile, const char __user *pUser_buff, size_t count, loff_t *pOffset){
    size_t bytes_to_copy, not_copied, copied;
    
    //aquire the mutex and tell the kernel that the syscall to restart the syscall if needed when the mutex is taken already
    if(mutex_lock_interruptible(&dev_mutex)) return -ERESTARTSYS;

    if(*pOffset >= DEV_BUFFER_SIZE){
        pr_info("%s: no space left to write\n", my_device);
        mutex_unlock(&dev_mutex);
        return -ENOSPC; 
    }else if(count > DEV_BUFFER_SIZE - (size_t)*pOffset){
        bytes_to_copy = DEV_BUFFER_SIZE - (size_t)*pOffset;
        pr_info("%s: only %zu bytes left to write\n", my_device, bytes_to_copy);
    }else{
        bytes_to_copy = count;
        pr_info("%s: writing to %zu bytes\n", my_device, bytes_to_copy);
    }

    pr_info("%s: write request=%zu, offset=%lld, will copy=%zu\n", my_device, count, *pOffset, bytes_to_copy);

    not_copied = copy_from_user(dev_buffer + *pOffset, pUser_buff, bytes_to_copy);

    copied = bytes_to_copy - not_copied;

    if(not_copied){
        pr_warn("%s: copy_from_user: only copied %zu/%zu\n", my_device, copied, bytes_to_copy);
    }

    *pOffset += copied;

    pr_info("%s: write done: copied=%zu, new_offset=%lld\n", my_device, copied, *pOffset);

    mutex_unlock(&dev_mutex);
    return (size_t)copied;

}

static struct file_operations fops = {
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init my_init(void){
    int status;

    //allocate memory, GFP_KERNEL means that allocation is done in normal kernel context
    dev_buffer = kmalloc(DEV_BUFFER_SIZE, GFP_KERNEL);
    if(!dev_buffer){
        pr_err("%s: Failed to allocate device buffer\n", my_device);
        return -ENOMEM;
    }
    //initialize mutex
    mutex_init(&dev_mutex);
    //clear device buffer
    memset(dev_buffer, 0, DEV_BUFFER_SIZE);

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