#ifndef _CLASS_H_
#define _CLASS_H_


#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>

#include "configuration.h"
#include "ioctl_command.h"
#include "macros.h"
#include "buffer.h"


// * _ EXTERN GLOBAL VARIABLE DEFINITION _______________________________________

extern FIFO_t   fifos[FIFO_DEV_COUNT]; 


// * _ CLASS DEVICE FUNCTIONS __________________________________________________

/// @brief sys/class read function to shows buffer content through the 
///        sys/class/fifo*/buffer file. 
/// @param dev  pointer to a device struct. 
/// @param attr not used. 
/// @param buf  buffer where we'll print the buffer view. 
/// @return     the number of bytes printed into the sysfs file. 
ssize_t fifo_buffer_show(struct device *dev, struct device_attribute *attr, char *buf); 


/// @brief sys/class read function to shows the free space available in the 
///        buffer. 
/// @param dev  pointer to a device struct. 
/// @param attr not used. 
/// @param buf  buffer where we'll print the free space in bytes. 
/// @return     the number of bytes printed into the sysfs file. 
ssize_t fifo_free_space_show(struct device *dev, struct device_attribute *attr, char *buf); 


/// @brief sys/class read function to shows the used space available in the 
///        buffer. 
/// @param dev  pointer to a device struct. 
/// @param attr not used. 
/// @param buf  buffer where we'll print the used space in bytes. 
/// @return     the number of bytes printed into the sysfs file. 
ssize_t fifo_used_space_show(struct device *dev, struct device_attribute *attr, char *buf); 


#endif