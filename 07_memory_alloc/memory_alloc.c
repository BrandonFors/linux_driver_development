
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

MODULE_LICENSE("GLP");
MODULE_AUTHOR("BrandonFors");
MODULE_DESCRIPTION("Memory allocation example");

static char *kmalloc_ptr;
static char *kzalloc_ptr;
static char *vmalloc_ptr;

#define ALLOC_SIZE_SMALL (1 << 10) //small allocation
#define ALLOC_SIZE_LARGE (1 << 20) // large allocation

struct my_obj{
    int id;
    char name[32];
};

static struct kmem_cache *my_cache;
static struct my_obj *obj1;

static int __init my_init(void){

    // kmalloc -> physically contiguous, no zeroing
    kmalloc_ptr = kmalloc(ALLOC_SIZE_SMALL, GFP_KERNEL);
    if(!kmalloc_ptr){
        pr_err("kmalloc failed\n");
        return -ENOMEM;
    }
    pr_info("kmalloc allocated at %px (physical %llx)\n", kmalloc_ptr, virt_to_phys(kmalloc_ptr));

    //kzalloc -> same as kmalloc but zeros memory
    kzalloc_ptr = kmalloc(ALLOC_SIZE_SMALL, GFP_KERNEL);
    if(!kzalloc_ptr){
        pr_err("kzalloc failed\n");
        goto label_kmalloc_ptr_free;
    }

    pr_info("kzalloc allocated at %px\n", kzalloc_ptr);

    // vmalloc: virtually contiguous, suitable for large blocks
    vmalloc_ptr = vmalloc(ALLOC_SIZE_LARGE);
    if (!vmalloc_ptr){
        pr_err("vmalloc failed\n");
        goto label_kzalloc_ptr_free;
    }
    pr_info("vmalloc allocated at %px\n", vmalloc_ptr);

    // slab allocation
    // if our object is a standard size used in an existing slab cache, the kernel
    //will merge our cache with the existing one. If we want to prevent, this 
    // we add the SLAB_NO_MERGE flag
    my_cache = kmem_cache_create("my_cache", 
                                 sizeof(struct my_obj), 
                                 0, 
                                 SLAB_HWCACHE_ALIGN | SLAB_NO_MERGE, 
                                 NULL);

    if(!my_cache) goto label_vmalloc_ptr_free;

    obj1 = kmem_cache_alloc(my_cache, GFP_KERNEL);
    if(!obj1){
        goto label_kmem_cache_free;
    }
    
    obj1->id = 1;
    strcpy(obj1->name, "slab_obj");
    
    pr_info("Slab object allocated at %px\n", obj1);

    return 0;

    label_kmem_cache_free:
        kmem_cache_destroy(my_cache);
    label_vmalloc_ptr_free:
        vfree(vmalloc_ptr);
    label_kzalloc_ptr_free:
        kfree(kzalloc_ptr);
    label_kmalloc_ptr_free:
        kfree(kmalloc_ptr);
        
    return -ENOMEM;

}

static void __exit my_exit(void){
    kfree(kmalloc_ptr);
    kfree(kzalloc_ptr);
    vfree(vmalloc_ptr);
    kmem_cache_destroy(my_cache);

    pr_info("Memory free\n");
}

module_init(my_init);
module_exit(my_exit);