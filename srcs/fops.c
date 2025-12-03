#include "fops.h"



ssize_t fifo_read(struct file* fp, char __user* buf, size_t nbc, loff_t* pos)
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

    return been_read; 
}


ssize_t fifo_write(struct file* fp, const char __user* buf, size_t nbc, loff_t* pos)
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


long int fifo_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
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