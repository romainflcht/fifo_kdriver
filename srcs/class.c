#include "class.h"


ssize_t fifo_buffer_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    dev_t   devno; 
    int     minor; 
    ssize_t offset; 
    bool    buf_overflow; 
    int     i; 

    devno = dev->devt; 
    minor = MINOR(devno); 

    // Check if the buffer is too large to print through sysfs. If it is, 
    // truncate it during printing. 
    buf_overflow = false; 
    if ((FIFO_BUFFER_SIZE * 2)+ 6 > PAGE_SIZE)
        buf_overflow = true; 

    offset = 0; 

    for (i = 0; i < FIFO_BUFFER_SIZE; i += 1)
    {
        // Is the buffer is too large, prints only firsts and lasts elements. 
        if (buf_overflow && i > ELT_CLASS_COUNT + 1 && 
            i < (FIFO_BUFFER_SIZE - 1) - ELT_CLASS_COUNT + 2)
            continue; 

        // Print ... for the last element to show in case the buffer is too 
        // large. 
        else if (buf_overflow && i == ELT_CLASS_COUNT)
        {
            offset += sysfs_emit_at(buf, offset, "|..."); 
            continue; 
        }

        // Print the current element unless it's 0. 
        else if ((fifos[minor].buffer)[i] > 0x20)
        {
            offset += sysfs_emit_at(buf, offset, "|%c", (fifos[minor].buffer)[i]); 
            continue; 
        }

        offset += sysfs_emit_at(buf, offset, "|@"); 
    }

    // Close the array. 
    offset += sysfs_emit_at(buf, offset, "|\n"); 
    INFO_DEBUG(
        "[FIFO] Buffer view requested through /sys/class/fifo%d/. "
        "Wrote %zu bytes.\n", 
        minor, 
        offset
    ); 
    return offset;
}


ssize_t fifo_free_space_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    dev_t   devno; 
    int     minor; 
    int     free_space; 

    // Get the minor number of the device. 
    devno = dev->devt; 
    minor = MINOR(devno); 

    // Calculate the available space and send it to the sysfs. 
    free_space = fifo_get_free_space(minor); 

    if (free_space < 0)
        return sysfs_emit(buf, "An error occurred while opening the device MINOR %d.\n", minor); 

    return sysfs_emit(buf, "%d\n", free_space); 
}


ssize_t fifo_used_space_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    dev_t   devno; 
    int     minor; 
    int     free_space; 

    // Get the minor number of the device. 
    devno = dev->devt; 
    minor = MINOR(devno); 

    // Calculate the available space and send it to the sysfs. 
    free_space = fifo_get_free_space(minor); 

    if (free_space < 0)
        return sysfs_emit(buf, "An error occurred while opening the device MINOR %d.\n", minor); 

    // Calculate the used space using the total buffer size and the free space. 
    return sysfs_emit(buf, "%d\n", FIFO_BUFFER_SIZE - free_space); 
}
