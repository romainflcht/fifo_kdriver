#include "buffer.h"


int init_fifo(FIFO_t* fifo, unsigned int minor, struct file_operations* fops)
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
        cdev_del(&(fifo->cdev));
        return -ENOMEM; 
    }

    // Create the device class device. 
    fifo->class_device = device_create(
        fifo_class, 
        NULL, 
        dev_minor, 
        NULL, 
        "fifo%c", 
        ('0' + minor)
    );

    if (IS_ERR(fifo->class_device))
    {
        kfree(fifo->buffer); 
        cdev_del(&(fifo->cdev));
        return PTR_ERR(fifo->class_device);
    }

    // Create the buffer view, free and used space left sys/class file. 
    device_create_file(fifo->class_device, &dev_attr_view);
    device_create_file(fifo->class_device, &dev_attr_free);
    device_create_file(fifo->class_device, &dev_attr_used);
    
    // Initialize mutexes and cursors. 
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


int fifo_reset(unsigned int minor)
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


int fifo_get_free_space(int minor)
{
    int r_cur; 
    int w_cur;
    int free_space; 

    // Check if the minor number is available. 
    if (minor > FIFO_DEV_COUNT - 1)
    {
        ERR_DEBUG("[FIFO] Trying to access an unregistered device.\n"); 
        return -ENODEV; 
    }
    
    r_cur = fifos[minor].r_cur; 
    w_cur = fifos[minor].w_cur;
    free_space = 0; 

    // If the read cursor is at it's initialization position, the buffer is 
    // empty or the write cursor has moved. Avoid an infinite loop. 
    if (r_cur < 0)
        return FIFO_BUFFER_SIZE - w_cur; 

    // Increment the space_left variable until the write cursor reach the read 
    // cursor. 
    while (w_cur != r_cur)
    {
        w_cur = (w_cur + 1) % FIFO_BUFFER_SIZE; 
        free_space += 1; 
    }
    
    return free_space + 1; 
}
