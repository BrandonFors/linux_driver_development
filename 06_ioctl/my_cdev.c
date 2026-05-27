#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BrandonFors - LDD");
MODULE_DESCRIPTION("First character device");

static const char *my_device = "my_cdev";

//assures user space and kernel space use the same command values
//a unique identifier used to group related io commands
#define MYCDEV_MAGIC        'M'
#define MYCDEV_CLEAR        _IO(MYCDEV_MAGIC, 1)
#define MYCDEV_SAY_HELLO    _IO(MYCDEV_MAGIC, 2)
#define MYCDEV_USER_READ    _IOR(MYCDEV_MAGIC, 3, int) //user read - kernel write
#define MYCDEV_USER_WRITE   _IOW(MYCDEV_MAGIC, 4, int) //user write - kernel read

//holds allocated device number, major and minor
static dev_t dev_nr;

static struct cdev my_cdev;

static struct class *my_class;

#define DEV_BUFFER_SIZE 64

static struct mutex dev_mutex;

static int my_open(struct inode *pInode, struct file *pFile){
    pr_info("%s: my_open called, Major: %d, Minor: %d\n", my_device, imajor(pInode), iminor(pInode));

    // with the private data approach, rather than a global dev buffer
    // memory is allocated when the device is opened
    // and it is freed when the device is closed
    // each instance of the device has its own buffer
    // memory is used only when it is needed
    //allocates memory and inits it to zero
    char *buffer = kzalloc(DEV_BUFFER_SIZE, GFP_KERNEL);

    if(!buffer) return -ENOMEM;

    pFile->private_data  = buffer;

    pr_info("%s: Allocated %d bytes for private data\n", my_device, DEV_BUFFER_SIZE);

    return 0;
}
static int my_release(struct inode *pInode, struct file *pFile){

    //free memory when device is closed
    char *buffer = (char *)pFile->private_data;
    if(buffer){
        kfree(buffer);
    }

    pr_info("%s: my_release called, freed %d bytes and file is closed\n", my_device, DEV_BUFFER_SIZE);

    return 0;
}

static ssize_t my_read(struct file *pFile, char __user *pUser_buff, size_t count, loff_t *pOffset){
    size_t bytes_to_copy, not_copied, copied;
    
    char *dev_buffer = (char *)pFile->private_data;
    if(!dev_buffer){
        return -EINVAL;
    }

    //aquire the mutex and tell the kernel that the syscall to restart the syscall if needed when the mutex is taken already
    if(mutex_lock_interruptible(&dev_mutex)) return -ERESTARTSYS;

    pr_info("%s: read called: request=%zu\n", my_device, count);

    bytes_to_copy = (count > strlen(dev_buffer)) ? (strlen(dev_buffer)) : count;

    pr_info("%s: Read will copy %zu bytes\n", my_device, bytes_to_copy);

    //returns number of bytes that could not be copied
    not_copied = copy_to_user(pUser_buff, dev_buffer, bytes_to_copy);
    copied = bytes_to_copy - not_copied;

    if(not_copied) pr_warn("%s: copy_to_user only copied %zu/%zu\n", my_device, copied, bytes_to_copy);

    pr_info("%s: Read done: return=%zu\n", my_device, copied);

    mutex_unlock(&dev_mutex);
    return (ssize_t) copied;
}
static ssize_t my_write(struct file *pFile, const char __user *pUser_buff, size_t count, loff_t *pOffset){
    size_t bytes_to_copy, not_copied, copied;
    
    char *dev_buffer = (char *)pFile->private_data;
    if(!dev_buffer){
        return -EINVAL;
    }

    //only write as much data as the buffer can hold
    bytes_to_copy = count > DEV_BUFFER_SIZE ? DEV_BUFFER_SIZE : count;

    //aquire the mutex and tell the kernel that the syscall to restart the syscall if needed when the mutex is taken already
    if(mutex_lock_interruptible(&dev_mutex)) return -ERESTARTSYS;

    pr_info("%s: write request=%zu, will copy=%zu\n", my_device, count, bytes_to_copy);

    not_copied = copy_from_user(dev_buffer, pUser_buff, bytes_to_copy);

    copied = bytes_to_copy - not_copied;

    if(not_copied){
        pr_warn("%s: copy_from_user: only copied %zu/%zu\n", my_device, copied, bytes_to_copy);
    }

    pr_info("%s: write done: copied=%zu\n", my_device, copied);

    mutex_unlock(&dev_mutex);
    return (size_t)copied;

}

static long my_ioctl (struct file *pFile, unsigned int cmd, unsigned long arg){
    int value;
    switch(cmd){
        case MYCDEV_CLEAR:
            char *dev_buffer = (char *)pFile->private_data;
            if(!dev_buffer){
                return -EINVAL;
            }
            memset(dev_buffer, 0, DEV_BUFFER_SIZE);
            pr_info("%s: private dat field cleared\n", my_device);
            break;
        case MYCDEV_SAY_HELLO:
            pr_info("%s: Hello from kernel via ioctl\n", my_device);
            return 0;
        case MYCDEV_USER_READ: // user read - kernel write
            value = 0xc0ffee;
            //__user tag says this belongs to user space, mainly for documentation and static analysis
            if(copy_to_user((int __user *)arg, &value, sizeof(int))){
                pr_err("%s, failed to copy value to user\n", my_device);
            }
            pr_info("%s: value: 0x%x copied to user \n", my_device, value);
            return 0;
        case MYCDEV_USER_WRITE: //user write - kernel read
            //__user tag says this belongs to user space, mainly for documentation and static analysis
            if(copy_from_user( &value,(int __user *)arg, sizeof(int))){
                pr_err("%s, failed to copy value from user\n", my_device);
            }
            pr_info("%s: value: 0x%x copied from user \n", my_device, value);
            return 0;
        default:
            pr_err("%s: unknown ioctl command 0x%x\n", my_device, cmd);
            return  -EINVAL;
            
    }
    return 0;
}

static struct file_operations fops = {
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .unlocked_ioctl = my_ioctl,
};

static int __init my_init(void){
    int status;

    //initialize mutex
    mutex_init(&dev_mutex);

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