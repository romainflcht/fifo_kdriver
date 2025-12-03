/*
TODO: support non blocking mode -> return -EAGAIN without writing (free space detection). 
*/

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>

#include "configuration.h"
#include "ioctl_command.h"
#include "macros.h"
#include "buffer.h"
#include "class.h"
#include "fops.h"


// * _ INITIALIZATION & EXIT FUNCTION DEFINITIONS ______________________________

/// @brief Module initialization function.
int __init fifo_init(void); 

#ifdef MODULE_COMPILATION
    /// @brief Module exit function.
    static void __exit fifo_exit(void); 
#endif


MODULE_LICENSE("GPL"); 
MODULE_VERSION(DRIVER_VERSION);
MODULE_AUTHOR("Romain FLACHAT"); 
MODULE_DESCRIPTION(
    "Character device driver implementing /dev/fifo* as a blocking FIFO with "
    "mutex-protected read/write. Exposes FIFO status via sysfs "
    "(/sys/class/fifo/*: used, free, view) and provides ioctl interface for "
    "buffer reset and cursor position retrieval."
);



// * _ GLOBAL VARIABLES ________________________________________________________

// Create a "wait_queue_head" structure named w_wait_queue. 
DECLARE_WAIT_QUEUE_HEAD(w_wait_queue);

// Create a "device_attribute" structure named dev_attr_buffer. 
DEVICE_ATTR(view, 0444, fifo_buffer_show, NULL);
DEVICE_ATTR(free, 0444, fifo_free_space_show, NULL);
DEVICE_ATTR(used, 0444, fifo_used_space_show, NULL);

// File operation structure used by the driver. 
struct file_operations fifo_fops = {
    .owner          = THIS_MODULE, 
    .read           = fifo_read,
    .write          = fifo_write,
    .unlocked_ioctl = fifo_ioctl, 
    .compat_ioctl   = fifo_ioctl, 
};

unsigned int    fifo_major = FIFO_MAJOR_NUMBER; 
FIFO_t          fifos[FIFO_DEV_COUNT]; 
struct class*   fifo_class;
bool            w_is_unlock; 


// * _ MODULE ENTRY POINT ______________________________________________________

#ifdef MODULE_COMPILATION
    module_init(fifo_init); 
    module_exit(fifo_exit); 
#endif


// * _ INITIALIZATION & EXIT FUNCTIONS _________________________________________

int __init fifo_init(void)
{
    dev_t   devno;
    int     retval; 
    int     i; 
    
    devno = MKDEV(fifo_major, 0);
    
    // Register major number and devices. 
    if (fifo_major)
        retval = register_chrdev_region(devno, FIFO_DEV_COUNT, "fifo");

    else 
        retval = alloc_chrdev_region(&devno, 0, FIFO_DEV_COUNT, "fifo");

    if (retval < 0)
    {
        ERR_DEBUG("[FIFO] error while allocating MAJOR, exiting...\n"); 
        return -ENODEV; 
    }

    // Save the major number. 
    fifo_major = MAJOR(devno);
    INFO_DEBUG("[FIFO] MAJOR allocation successful, MAJOR is %d.\n", MAJOR(devno)); 

    fifo_class = class_create("fifo");
    if (IS_ERR(fifo_class))
        return PTR_ERR(fifo_class);

    // Initialize the FIFO_t structure by allocation buffer memory and and 
    // minor device. 
    for (i = 0; i < FIFO_DEV_COUNT; i += 1)
    {
        retval = init_fifo(&(fifos[i]), i, &fifo_fops); 
        if (retval)
            return -EFAULT; 
    } 

    w_is_unlock = true; 
    printk(KERN_INFO "[FIFO] driver loaded successfully!\n"); 
    return 0; 
}


#ifdef MODULE_COMPILATION
    static void __exit fifo_exit(void)
    {
        int i; 

        // Unload each devices, free allocated memory, destroy class devices
        // and unregister the MAJOR. 
        for (i = 0; i < FIFO_DEV_COUNT; i += 1)
        {
            cdev_del(&(fifos[i].cdev)); 
            device_destroy(fifo_class, MKDEV(fifo_major, i));
            kfree(fifos[i].buffer); 
        } 

        class_destroy(fifo_class);

        unregister_chrdev_region(MKDEV(fifo_major, 0), FIFO_DEV_COUNT); 
        printk(KERN_INFO "[FIFO] driver unloaded successfully, goodbye!\n"); 
        return; 
    }
#endif
