#ifndef _IOCTL_COMMAND_H_
#define _IOCTL_COMMAND_H_

#include <linux/ioctl.h>

#define FIFO_MAGIC 0x40

#define IO_FIFO_RESET      _IO(FIFO_MAGIC, 0)
#define IO_FIFO_GET_R_CUR  _IOR(FIFO_MAGIC, 1, int)
#define IO_FIFO_GET_W_CUR  _IOR(FIFO_MAGIC, 2, int)

#endif