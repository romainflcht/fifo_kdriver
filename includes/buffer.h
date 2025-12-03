#ifndef _BUFFER_H_
#define _BUFFER_H_

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


// * _ STRUCTURE DEFINITIONS ___________________________________________________

typedef struct fifo_t
{
    struct cdev     cdev; 
    struct mutex    r_mutex; 
    struct mutex    w_mutex; 
    struct device*  class_device;
    unsigned char*  buffer;
    int             r_cur; 
    int             w_cur; 
}   FIFO_t; 


// * _ EXTERN GLOBAL VARIABLE DEFINITION _______________________________________

extern unsigned int             fifo_major; 
extern struct class*            fifo_class; 
extern struct device_attribute  dev_attr_view;
extern struct device_attribute  dev_attr_free;
extern struct device_attribute  dev_attr_used;
extern FIFO_t                   fifos[FIFO_DEV_COUNT]; 


// * _ FUNCTION DECLARATIONS ___________________________________________________

/// @brief Initialize the FIFO_t structure. 
/// @param fifo  pointer to a fifo structure. 
/// @param minor minor number that will correspond to that FIFO_t device. 
/// @param fops  file operation linked to that FIFO_t structure. 
/// @return 0 if no error occurred, negative otherwise. 
int init_fifo(FIFO_t* fifo, unsigned int minor, struct file_operations* fops);


/// @brief Reset the fifo buffer, empty it and reset read & write cursor 
///        position. 
/// @param minor minor number of the fifo to reset. 
int fifo_reset(unsigned int minor); 


/// @brief Return the number of bytes available to write. 
/// @param minor minor of the device we want to check. 
/// @return the free space in bytes. 
int fifo_get_free_space(int minor); 


/// @brief Return the number of bytes used in the buffer. 
/// @param minor minor of the device we want to check. 
/// @return the used space in bytes. 
int fifo_get_used_space(int minor); 

#endif