#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "ioctl_command.h"

#define INTERFACE "/dev/fifo0"


int main(int argc, char** argv)
{
    int fd; 
    int w_cur = 1; 
    int r_cur = 1; 

    fd = open(INTERFACE, O_RDWR);
    if (fd < 0)
    {
        printf("Error occurred while opening %s...\n", INTERFACE); 
        return -1; 
    }


    ioctl(fd, IO_FIFO_RESET); 
    
    ioctl(fd, IO_FIFO_GET_R_CUR, &r_cur); 
    ioctl(fd, IO_FIFO_GET_W_CUR, &w_cur);
    
    printf("r_cur: %d\n", r_cur); 
    printf("w_cur: %d\n", w_cur); 
    return 0; 
}