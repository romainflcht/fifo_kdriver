#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>


MODULE_LICENSE("GPL"); 

// * _ DEFINES _________________________________________________________________

#define FIFO_DEV_COUNT      3
#define FIFO_MAJOR_NUMBER   0
#define FIFO_BUFFER_SIZE    4096


// * _ STRUCTURE DEFINITIONS ___________________________________________________

typedef struct fifo_t
{
    struct cdev*    cdev; 
    char*           buffer;
    unsigned int    index; 
}   FIFO_t; 


struct file_operations fifo_fops = {
    .owner = THIS_MODULE, 
};


// * _ STATIC FUNCTION DEFINITIONS _____________________________________________

/// @brief Initialize the FIFO_t structure. 
/// @param fifo pointer to a fifo structure. 
/// @param minor_number minor number that will correspond to that FIFO_t device. 
/// @param fops file operation linked to that FIFO_t structure. 
/// @return 0 if no error occurred, negative otherwise. 
static int init_cdev_fifo(FIFO_t* fifo, unsigned int minor_number, struct file_operations* fops);


// * _ GLOBAL VARIABLES ________________________________________________________
unsigned int    fifo_major = FIFO_MAJOR_NUMBER; 
FIFO_t          fifos[FIFO_DEV_COUNT]; 


static int __init fifo_init(void)
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
        printk("[FIFO] error while allocating MAJOR, exiting...\n"); 
        return -ENODEV; 
    }

    // Save the major number. 
    fifo_major = MAJOR(devno);

    printk("[FIFO] MAJOR allocation successful, MAJOR is %d.\n", MAJOR(devno)); 
 

    // Initialize the FIFO_t structure by allocation buffer memory and and 
    // minor device. 
    // for (i = 0; i < FIFO_DEV_COUNT; i += 1)
    // {
    //     retval = init_cdev_fifo(&(fifos[i]), i, &fifo_fops); 
    //     if (retval)
    //         return -1; 
    // } 


    printk(KERN_INFO "[FIFO] driver loaded successfully!\n"); 
    return 0; 
}


static void __exit fifo_exit(void)
{
    unregister_chrdev_region(MKDEV(fifo_major, 0), FIFO_DEV_COUNT); 
    printk(KERN_INFO "[FIFO] driver unloaded successfully, goodbye!\n"); 
    return; 
}


module_init(fifo_init); 
module_exit(fifo_exit); 


// * _ STATIC FUNCTIONS ________________________________________________________

static int init_cdev_fifo(FIFO_t* fifo, unsigned int minor_number, struct file_operations* fops)
{
    int     retval; 
    dev_t   dev_minor; 

    dev_minor = MKDEV(fifo_major, minor_number); 

    // Initialize the cdev associated to the FIFO_t structure and minor number. 
    cdev_init(fifo->cdev, fops); 
    fifo->cdev->owner = THIS_MODULE;
    fifo->cdev->ops = fops;

    retval = cdev_add(fifo->cdev, dev_minor, 1);
    if (retval)
    {
        printk(KERN_ERR "[FIFO] device %d not added correctly, abort.\n", minor_number);
        return -ENODEV; 
    }

    fifo->index = 0; 
    fifo->buffer = kmalloc(FIFO_BUFFER_SIZE, GFP_KERNEL); 
    if (!fifo->buffer)
    {
        printk(KERN_ERR "[FIFO] device %d buffer not allocated correctly, abort.\n", minor_number);
        return -ENOMEM; 
    }

    return 0; 
}