#ifndef _FOPS_H_
#define _FOPS_H_

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


// * _ EXTERN GLOBAL VARIABLE DEFINITION _______________________________________

extern struct wait_queue_head   w_wait_queue; 
extern FIFO_t                   fifos[FIFO_DEV_COUNT]; 
extern bool                     w_is_unlock; 


// * _ FILE OPERATION FUNCTIONS ________________________________________________

/// @brief read file operation override. 
/// @param fp  pointer to the file structure. 
/// @param buf user-space buffer to put read data. 
/// @param nbc number of character the process wants to read. 
/// @param pos position of the read cursor. 
/// @return    the number of bytes returned by kernel space. 
ssize_t fifo_read(struct file* fp, char __user* buf, size_t nbc, loff_t* pos); 


/// @brief write file operation override. 
/// @param fp  pointer to the file structure. 
/// @param buf user-space buffer containing what needs to be written. 
/// @param nbc number of character the process is writting. 
/// @param pos position of the write cursor. 
/// @return    the number of bytes returned by kernel space. 
ssize_t fifo_write(struct file* fp, const char __user* buf, size_t nbc, loff_t* pos); 


/// @brief ioctl file operation override. 
/// @param inode pointer the the inode structure. 
/// @param fp    pointer the the file structure. 
/// @param cmd   command to execute. Definitions of every commands are in the 
///              file "ioctl_command.h". 
/// @param arg   argument of the command sent by the process. 
/// @return      0 if no error occurred, error code otherwise. 
long int fifo_ioctl(struct file *fp, unsigned int cmd, unsigned long arg); 

#endif 