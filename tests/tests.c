#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>

#include "ioctl_command.h"

#define INTERFACE "/dev/fifo0"

// * _ COMMANDS ________________________________________________________________
#define CMD_READ    "read"
#define CMD_WRITE   "write"
#define CMD_SET     "ioctl"

// * _ SET COMMANDS ____________________________________________________________
#define RESET           "reset"
#define GET_READ_CUR    "cursor"

// * _ FUNCTION DEFINITIONS ____________________________________________________
void test_read(int fd, char* str);
void test_write(int fd, char* str);
void test_set(int fd, char* str);
void usage(char* bin_name); 



int main(int argc, char** argv)
{
    int fd; 

    if (argc < 3)
    {
        usage(argv[0]); 
        return -1; 
    }

    fd = open(INTERFACE, O_RDWR);
    if (fd < 0)
    {
        printf("Error occurred while opening %s...\n", INTERFACE); 
        return -1; 
    }


    // ioctl(fd, IO_FIFO_RESET); 
    
    // ioctl(fd, IO_FIFO_GET_R_CUR, &r_cur); 
    // ioctl(fd, IO_FIFO_GET_W_CUR, &w_cur);
    
    // printf("r_cur: %d\n", r_cur); 
    // printf("w_cur: %d\n", w_cur); 
    
    if (!strcmp(argv[1], CMD_READ))
        test_read(fd, argv[2]); 

    else if (!strcmp(argv[1], CMD_WRITE))
        test_write(fd, argv[2]); 

    else if (!strcmp(argv[1], CMD_SET))
        test_set(fd, argv[2]);
    
    else 
        usage(argv[0]); 

    close(fd); 
    return 0; 
}


void test_read(int fd, char* str)
{
    int     count;
    size_t  retval;  
    char*   buf; 

    count = atoi(str); 
    if (count < 1)
        return; 

    buf = (char*)malloc(sizeof(char) * count); 
    if (!buf)
        return; 

    retval = read(fd, buf, count);
    printf("~Read bytes (%zu): %s\n", retval, buf); 
    
    free(buf); 
    return; 
}


void test_write(int fd, char* str)
{
    int     len; 
    size_t  retval; 
    
    len = strlen(str); 

    if (len < 1)
        return;

    retval = write(fd, str, len); 
    printf("~Wrote bytes (%zu): %s\n", retval, str);
    return; 
}


void test_set(int fd, char* str)
{
    int r_cur; 
    int w_cur;
    

    if (!strcmp(str, RESET))
    {
        ioctl(fd, IO_FIFO_RESET); 
        printf("~FIFO reset successful.\n"); 
    }

    else if (!strcmp(str, GET_READ_CUR))
    {
        ioctl(fd, IO_FIFO_GET_R_CUR, &r_cur); 
        ioctl(fd, IO_FIFO_GET_W_CUR, &w_cur);
        printf("~Cursor positions: r:%d | w:%d.\n", r_cur, w_cur); 
    }

    return; 
}


// * _ UTILITIES _______________________________________________________________


void usage(char* bin_name)
{
    printf("USAGE: \n\t %s [command] [arg]\n", bin_name); 
    return; 
}