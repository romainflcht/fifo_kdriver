/*
TODO: file in sys/class/fifo0/size, free, used. 
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

MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("Romain FLACHAT"); 


// * _ STRUCTURE DEFINITIONS ___________________________________________________

typedef struct fifo_t
{
    struct cdev     cdev; 
    struct mutex    r_mutex; 
    struct mutex    w_mutex; 
    char*           buffer;
    int             r_cur; 
    int             w_cur; 
}   FIFO_t; 


// * _ STATIC FUNCTION DEFINITIONS _____________________________________________

/// @brief Module initialization function
int __init fifo_init(void); 



/// @brief Initialize the FIFO_t structure. 
/// @param fifo  pointer to a fifo structure. 
/// @param minor minor number that will correspond to that FIFO_t device. 
/// @param fops  file operation linked to that FIFO_t structure. 
/// @return 0 if no error occurred, negative otherwise. 
static int init_cdev_fifo(FIFO_t* fifo, unsigned int minor, struct file_operations* fops);


/// @brief Reset the fifo buffer, empty it and reset read & write cursor 
///        position. 
/// @param minor minor number of the fifo to reset. 
static int fifo_reset(unsigned int minor); 


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


/// @brief ioctl file operation override. 
/// @param inode pointer the the inode structure. 
/// @param fp    pointer the the file structure. 
/// @param cmd   command to execute. Definitions of every commands are in the 
///              file "ioctl_command.h". 
/// @param arg   argument of the command sent by the process. 
/// @return      0 if no error occurred, error code otherwise. 
static long int fifo_ioctl(struct file *fp, unsigned int cmd, unsigned long arg); 


// * _ FILE OPERATION __________________________________________________________

struct file_operations fifo_fops = {
    .owner          = THIS_MODULE, 
    .read           = fifo_read,
    .write          = fifo_write,
    .unlocked_ioctl = fifo_ioctl, 
    .compat_ioctl   = fifo_ioctl, 
};


// * _ GLOBAL VARIABLES ________________________________________________________
static          DECLARE_WAIT_QUEUE_HEAD(w_wait_queue);
unsigned int    fifo_major = FIFO_MAJOR_NUMBER; 
FIFO_t          fifos[FIFO_DEV_COUNT]; 
bool            w_is_unlock; 


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

    // Initialize the FIFO_t structure by allocation buffer memory and and 
    // minor device. 
    for (i = 0; i < FIFO_DEV_COUNT; i += 1)
    {
        retval = init_cdev_fifo(&(fifos[i]), i, &fifo_fops); 
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

        // Unload each devices, free allocated memory and unregister the MAJOR. 
        for (i = 0; i < FIFO_DEV_COUNT; i += 1)
        {
            cdev_del(&(fifos[i].cdev)); 
            kfree(fifos[i].buffer); 
        } 

        unregister_chrdev_region(MKDEV(fifo_major, 0), FIFO_DEV_COUNT); 
        printk(KERN_INFO "[FIFO] driver unloaded successfully, goodbye!\n"); 
        return; 
    }
#endif

#ifdef MODULE_COMPILATION
    module_init(fifo_init); 
    module_exit(fifo_exit); 
#endif


// * _ STATIC FUNCTIONS ________________________________________________________

static int init_cdev_fifo(FIFO_t* fifo, unsigned int minor, struct file_operations* fops)
{
    dev_t   dev_minor; 
    int     retval; 
    int     i; 

    dev_minor = MKDEV(fifo_major, minor); 

    // Initialize the cdev associated to the FIFO_t structure and minor number. 
    cdev_init(&(fifo->cdev), fops); 
    fifo->cdev.owner = THIS_MODULE;
    fifo->cdev.ops = fops;

    // Register the character device. 
    retval = cdev_add(&(fifo->cdev), dev_minor, 1);
    if (retval)
    {
        ERR_DEBUG("[FIFO] device %d not added correctly, abort.\n", minor);
        return -ENODEV; 
    }

    // Allocate buffer memory and set read/write index to 0. 
    fifo->buffer = (char*)kmalloc(FIFO_BUFFER_SIZE, GFP_KERNEL); 
    if (!fifo->buffer)
    {
        ERR_DEBUG("[FIFO] device %d buffer not allocated correctly, abort.\n", minor);
        return -ENOMEM; 
    }
    
    mutex_init(&(fifo->r_mutex)); 
    mutex_init(&(fifo->w_mutex)); 
    fifo->r_cur = -1; 
    fifo->w_cur = 0; 

    // Fill the buffer with zeros. 
    for (i = 0; i < FIFO_BUFFER_SIZE; i += 1)
        fifo->buffer[i] = 0; 
    
    INFO_DEBUG("[FIFO] device %d is correctly registered.\n", minor);
    return 0; 
}


static int fifo_reset(unsigned int minor)
{
    int i; 

    // Lock the read and write mutex while resetting the buffer. 
    if (mutex_lock_interruptible(&(fifos[minor].r_mutex)))
        return -ERESTARTSYS;

    if (mutex_lock_interruptible(&(fifos[minor].w_mutex)))
        return -ERESTARTSYS;

    // Empty the fifo buffer. 
    for (i = 0; i < FIFO_BUFFER_SIZE; i += 1)
        fifos[minor].buffer[i] = 0; 

    // Reset cursor position. 
    fifos[minor].r_cur = -1; 
    fifos[minor].w_cur = 0; 

    // Unlock both mutexes. 
    mutex_unlock(&(fifos[minor].r_mutex)); 
    mutex_unlock(&(fifos[minor].w_mutex)); 
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
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0) 
        minor = iminor(file_inode(fp)); 
    #else
        minor = MINOR(fp->f_path.dentry->d_inode->i_rdev);
    #endif
    
    if (minor > FIFO_DEV_COUNT - 1)
    {
        ERR_DEBUG("[FIFO] Trying to access an unregistered device.\n"); 
        return -ENODEV; 
    }

    INFO_DEBUG(
        "[FIFO] %zu byte(s) read operation asked from MINOR %d, "
        "read_cursor currently at %d\n", nbc, minor, fifos[minor].r_cur
    ); 

    // If the read cursor is already on write cursor, read nothing. 
    if ((fifos[minor].r_cur + 1) % FIFO_BUFFER_SIZE == fifos[minor].w_cur)
        return 0; 

    // Else, allocate a kernel buffer to read the fifo buffer. 
    kbuf = (char*)kmalloc(nbc, GFP_KERNEL); 
    if (!kbuf)
        return -ENOMEM; 

    // Protect the read operation from other concurrent process by locking the 
    // read mutex. 
    if (mutex_lock_interruptible(&(fifos[minor].r_mutex)))
        return -ERESTARTSYS;

    // Read the buffer we reach the number of bytes we want to read or until we 
    // reach the write cursor. 
    been_read = 0; 
    to_read = nbc; 
    while (to_read > 0)
    {
        fifos[minor].r_cur = (fifos[minor].r_cur + 1) % FIFO_BUFFER_SIZE; 
        kbuf[been_read] = (fifos[minor].buffer)[fifos[minor].r_cur]; 

        been_read += 1; 
        to_read -= 1; 

        // If we reach the write cursor, stop the read. 
        if (((fifos[minor].r_cur + 1) % FIFO_BUFFER_SIZE) == fifos[minor].w_cur)
            break; 
    }

    if (!w_is_unlock)
    {
        w_is_unlock = true; 
        wake_up_interruptible(&w_wait_queue); 
    }

    // Copy the kernel buffer into user-space buffer, free the kernel buffer 
    // and return the number of bytes returned. 
    retval = copy_to_user(buf, kbuf, been_read);
    kfree(kbuf); 

    if (retval) 
        return -EFAULT; 
    
    // Unlock the read mutex. 
    mutex_unlock(&(fifos[minor].r_mutex));
    
    INFO_DEBUG(
        "[FIFO] %zu byte(s) returned to MINOR %d, read_cursor "
        "currently at %d.\n", been_read, minor, fifos[minor].r_cur
    );


    WARN_DEBUG("from R: r: %d, w: %d (%zu)", fifos[minor].r_cur, fifos[minor].w_cur, been_read); 
    return been_read; 
}


static ssize_t fifo_write(struct file* fp, const char __user* buf, size_t nbc, loff_t* pos)
{
    int             retval;
    unsigned int    minor;
    char*           kbuf;
    size_t          i; 

    // Get the device minor number that asked the write. 
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0) 
        minor = iminor(file_inode(fp)); 
    #else
        minor = MINOR(fp->f_path.dentry->d_inode->i_rdev);
    #endif

    if (minor > FIFO_DEV_COUNT - 1)
    {
        ERR_DEBUG("[FIFO] Trying to access an unregistered device.\n"); 
        return -ENODEV; 
    }

    INFO_DEBUG(
        "[FIFO] %zu byte(s) write operation asked from MINOR %d, "
        "write_cursor currently at %d\n", nbc, minor, fifos[minor].w_cur
    ); 

    // If the next write cursor is the read cursor, abort the read to not 
    // overwrite the read buffer. 
    if (fifos[minor].w_cur == fifos[minor].r_cur)
    {
        INFO_DEBUG("[FIFO] No space left to write, waiting for read.\n"); 
        w_is_unlock = false; 
        wait_event_interruptible(w_wait_queue, w_is_unlock);
    }

    // Allocate a kernel buffer and get user-space provided data. 
    kbuf = (char*)kmalloc(nbc, GFP_KERNEL);
    if (!kbuf)
        return -ENOMEM; 

    retval = copy_from_user(kbuf, buf, nbc); 
    if (retval)
    {
        kfree(kbuf); 
        return -EFAULT; 
    }

    // Protect the write operation from other concurrent process by locking the 
    // write mutex. 
    if (mutex_lock_interruptible(&(fifos[minor].w_mutex)))
        return -ERESTARTSYS;

    i = 0; 
    while (i < nbc)
    {
        // Check if the read cursor has moved from it's initialization. If not, 
        // stop the writting before it to not block the read cursor and lose the
        // initial write data. 
        if (fifos[minor].r_cur == -1 && fifos[minor].w_cur == FIFO_BUFFER_SIZE - 1)
        {
            INFO_DEBUG("[FIFO] No space left to write, waiting for read.\n"); 
            w_is_unlock = false; 
            wait_event_interruptible(w_wait_queue, w_is_unlock); 
        }

        fifos[minor].buffer[fifos[minor].w_cur] = kbuf[i]; 

        // If the next position is safe, increment the write cursor. 
        fifos[minor].w_cur = (fifos[minor].w_cur + 1) % FIFO_BUFFER_SIZE;
        i += 1; 

        // If the write cursor is positionned on the read cursor, no space left 
        // to write, block the execution. 
        if (fifos[minor].w_cur == fifos[minor].r_cur)
        {
            INFO_DEBUG("[FIFO] No space left to write, waiting for read.\n"); 
            w_is_unlock = false; 
            wait_event_interruptible(w_wait_queue, w_is_unlock); 
        }
    }

    INFO_DEBUG(
        "[FIFO] %zu byte(s) written to device with MINOR %d, "
        "write_cursor currently at %d.\n", nbc, minor, fifos[minor].w_cur
    ); 

    kfree(kbuf); 

    // Unlock the write mutex. 
    mutex_unlock(&(fifos[minor].w_mutex)); 
    return nbc; 
}


static long int fifo_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
    int             r_cur; 
    int             w_cur; 
    int             retval; 
    unsigned int    minor; 

    // Get the device minor number that need to be configured. 
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 9, 0) 
        minor = iminor(file_inode(fp)); 
    #else
        minor = MINOR(fp->f_path.dentry->d_inode->i_rdev);
    #endif

    if (minor > FIFO_DEV_COUNT - 1)
    {
        ERR_DEBUG("[FIFO] Trying to access an unregistered device.\n"); 
        return -ENODEV; 
    }

    switch(cmd)
    {
        case IO_FIFO_RESET: 
            // Reset the fifo read and write cursor and clear the buffer. 
            retval = fifo_reset(minor); 

            if (retval)
                return -EFAULT; 
        break; 

        case IO_FIFO_GET_R_CUR: 
            // Send the read cursor to the userspace. 
            r_cur = fifos[minor].r_cur; 
            retval = copy_to_user((int __user *)arg, &r_cur, sizeof(int));

            if (retval)
                return -EFAULT;
        break; 

        case IO_FIFO_GET_W_CUR: 
            // Send the write cursor to the userspace. 
            w_cur = fifos[minor].w_cur; 
            retval = copy_to_user((int __user *)arg, &w_cur, sizeof(int)); 

            if (retval)
                return -EFAULT;
        break; 

        default: 
            return -ENOTTY; 
    }

    return 0; 
}
