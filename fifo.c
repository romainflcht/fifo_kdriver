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
    struct cdev     cdev; 
    char*           buffer;
    int             index; 
}   FIFO_t; 


// * _ STATIC FUNCTION DEFINITIONS _____________________________________________

/// @brief Initialize the FIFO_t structure. 
/// @param fifo  pointer to a fifo structure. 
/// @param minor minor number that will correspond to that FIFO_t device. 
/// @param fops  file operation linked to that FIFO_t structure. 
/// @return 0 if no error occurred, negative otherwise. 
static int init_cdev_fifo(FIFO_t* fifo, unsigned int minor, struct file_operations* fops);


/// @brief read file operation override. 
/// @param fp  pointer to the file structure. 
/// @param buf user-space buffer to put read data. 
/// @param nbc number of character the process wants to read. 
/// @param pos position of the read cursor. 
/// @return    the number of bytes returned by kernel space. 
static ssize_t fifo_read(struct file* fp, char __user* buf, size_t nbc, loff_t* pos); 


/// @brief write file operation override. 
/// @param fp  pointer to the file structure. 
/// @param buf user-space buffer containing what needs to be written. 
/// @param nbc number of character the process is writting. 
/// @param pos position of the write cursor. 
/// @return    the number of bytes returned by kernel space. 
static ssize_t fifo_write(struct file* fp, const char __user* buf, size_t nbc, loff_t* pos); 


// * _ FILE OPERATION __________________________________________________________

struct file_operations fifo_fops = {
    .owner = THIS_MODULE, 
    .read  = fifo_read,
    .write = fifo_write,
};


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
    for (i = 0; i < FIFO_DEV_COUNT; i += 1)
    {
        retval = init_cdev_fifo(&(fifos[i]), i, &fifo_fops); 
        if (retval)
            return -EFAULT; 
    } 

    printk(KERN_INFO "[FIFO] driver loaded successfully!\n"); 
    return 0; 
}


static void __exit fifo_exit(void)
{
    int i; 

    // Unload each devices and unregister the MAJOR. 
    for (i = 0; i < FIFO_DEV_COUNT; i += 1)
        cdev_del(&(fifos[i].cdev)); 

    unregister_chrdev_region(MKDEV(fifo_major, 0), FIFO_DEV_COUNT); 
    printk(KERN_INFO "[FIFO] driver unloaded successfully, goodbye!\n"); 
    return; 
}


module_init(fifo_init); 
module_exit(fifo_exit); 


// * _ STATIC FUNCTIONS ________________________________________________________

static int init_cdev_fifo(FIFO_t* fifo, unsigned int minor, struct file_operations* fops)
{
    int     retval; 
    dev_t   dev_minor; 

    dev_minor = MKDEV(fifo_major, minor); 

    // Initialize the cdev associated to the FIFO_t structure and minor number. 
    cdev_init(&(fifo->cdev), fops); 
    fifo->cdev.owner = THIS_MODULE;
    fifo->cdev.ops = fops;

    // Register the character device. 
    retval = cdev_add(&(fifo->cdev), dev_minor, 1);
    if (retval)
    {
        printk(KERN_ERR "[FIFO] device %d not added correctly, abort.\n", minor);
        return -ENODEV; 
    }

    // Allocate buffer memory and set read/write index to 0. 
    fifo->buffer = (char*)kmalloc(FIFO_BUFFER_SIZE, GFP_KERNEL); 
    if (!fifo->buffer)
    {
        printk(KERN_ERR "[FIFO] device %d buffer not allocated correctly, abort.\n", minor);
        return -ENOMEM; 
    }
    
    fifo->index = 127; 

    for (int i = 0; i < 127; i += 1)
        fifo->buffer[i] = i; 
    
    printk(KERN_INFO "[FIFO] device %d is correctly registered.\n", minor);
    return 0; 
}


// * _ FILE OPERATION FUNCTIONS ________________________________________________

static ssize_t fifo_read(struct file* fp, char __user* buf, size_t nbc, loff_t* pos)
{
    unsigned int    minor;
    int             retval; 
    size_t          to_read; 
    size_t          been_read; 
    char*           kbuf; 

    // Get the device minor number that asked the read. 
    minor = iminor(fp->f_inode); 
    if (minor > FIFO_DEV_COUNT - 1)
    {
        printk(KERN_ERR "[FIFO] Trying to access an unregistered device.\n"); 
        return -ENODEV; 
    }
    
    // If the fifo is empty, return no character. 
    if (fifos[minor].index < 0)
        return 0; 
    
    // Else, allocate a kernel buffer to read the fifo buffer. 
    kbuf = kmalloc(nbc, GFP_KERNEL); 
    if (!kbuf)
        return -ENOMEM; 

    // Read the buffer until it's either empty of reach the number of bytes 
    // asked by the user-space process. 
    been_read = 0; 
    to_read = nbc; 
    while (to_read > 0 && fifos[minor].index >= 0)
    {
        kbuf[been_read] = (fifos[minor].buffer)[fifos[minor].index]; 
        
        fifos[minor].index -= 1; 
        to_read -= 1; 
        been_read += 1; 
    }

    // Copy the kernel buffer into user-space buffer, free the kernel buffer 
    // and return the number of bytes returned. 
    retval = copy_to_user(buf, kbuf, been_read);
    kfree(kbuf); 
    
    if (retval) 
        return -EFAULT; 
    
    printk(KERN_INFO "[FIFO] Device %d read %ld bytes.\n", minor, been_read); 
    return been_read; 
}


static ssize_t fifo_write(struct file* fp, const char __user* buf, size_t nbc, loff_t* pos)
{
    int             retval;
    size_t          i; 
    unsigned int    minor;
    char*           kbuf;

    // Get the device minor number that asked the read. 
    minor = iminor(fp->f_inode); 
    if (minor > FIFO_DEV_COUNT - 1)
    {
        printk(KERN_ERR "[FIFO] Trying to access an unregistered device.\n"); 
        return -ENODEV; 
    }

    // Allocate a kernel buffer and get user-space provided data. 
    kbuf = kmalloc(nbc, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM; 

    retval = copy_from_user(kbuf, buf, nbc); 
    if (retval)
        return -EFAULT; 


    for (i = 0; i < nbc; i += 1)
    {
        fifos[minor].index += 1; 
        fifos[minor].buffer[fifos[minor].index] = kbuf[i]; 
        
    }

    printk(KERN_INFO "[FIFO] write operation asked. %s\n", kbuf); 

    kfree(kbuf); 
    return nbc; 
}