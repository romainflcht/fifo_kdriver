# fifo_kdriver
Practical assignment character device driver implementing /dev/fifo* as a blocking FIFO with mutex-protected read/write. Exposes FIFO status via sysfs (/sys/class/fifo/*: used, free, view) and provides ioctl interface for buffer reset and cursor position retrieval.

## Configuration
The driver can be configured as you need it by tweaking the `configuration.h` file before compilation:
```c
/// @brief defines the debug level.
/// - 2: All debug prints will be displayed. 
/// - 1: Only errors debug will be displayed. 
/// - 0: Nothing will be printed. 
#define DEBUG                   1

// Defines the major number used by the kernel. 
#define FIFO_MAJOR_NUMBER       0

// Defines the number of minors handled by the driver. 
#define FIFO_DEV_COUNT          3

// Defines the total buffer size for each interface in bytes. 
#define FIFO_BUFFER_SIZE        2048

// Define the number of element to show when printing a graphical representation 
// of a fifo buffer.
// Example: selecting 10 will result in: 
// |h|e|y|!|@|@|@|@|@|@|...|l|@|@|@|@|@|@|@|@|@| 
//
// Example: selecting 5 will result in: 
// |h|e|y|!|@|...|l|@|@|@|@|@| 
#define ELT_CLASS_COUNT         5
```

## Compilation
To compile the driver as a module to insert into your kernel, execute the `Makefile` by running the command:
```bash
make

--KERNEL SPACE COMPILATION: 
~COMPILING fifo...
make[1]: Entering directory '/usr/src/linux-headers-6.14.0-36-generic'
make[2]: Entering directory '~/Developer/linux_kernel/fifo_kdriver'
warning: the compiler differs from the one used to build the kernel
  The kernel was built by: x86_64-linux-gnu-gcc-13 (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
  You are using:           gcc-13 (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
  CC [M]  main.o
  CC [M]  srcs/buffer.o
  CC [M]  srcs/fops.o
  CC [M]  srcs/class.o
  LD [M]  fifo.o
  MODPOST Module.symvers
  CC [M]  fifo.mod.o
  CC [M]  .module-common.o
  LD [M]  fifo.ko
  BTF [M] fifo.ko
Skipping BTF generation for fifo.ko due to unavailability of vmlinux
make[2]: Leaving directory '~/Developer/linux_kernel/fifo_kdriver'
make[1]: Leaving directory '/usr/src/linux-headers-6.14.0-36-generic'
~ DONE ~
``` 

You can also use the `Makefile` to clean the project directory, automatically insert the module when the compilation is finished or remove the module from the kernel with those three commands: 
```bash
make clean
```

```bash
make insert
~INSERTING fifo.ko INTO KERNEL 
```

```bash
make remove
~REMOVING fifo.ko FROM KERNEL
```

## How to use
The project comes with a test script located in `tests/tests.c` and can be compiled using the command:
```bash
make test
--USER SPACE COMPILATION: 
~COMPILING tests TO bin/tests
```

This script is capable of executing three type of operation and is used like this:
```bash
./tests -h
USAGE: 
	./tests [command] [arg]
```
> [!NOTE] 
> That this script performs those operation on only the first device named `/dev/fifo0`.

### read operation
To read the available content inside the FIFO buffer using the test script, use the `read` command followed by the number of bytes you want to read. 
```bash
tests read 4 
~Read bytes (4): hey!
```

### write operation
To write data inside the FIFO buffer using the test script, use the `write` command followed the string you want to write. 

```bash
./tests write hey! 
~Wrote bytes (4): hey!
```

### ioctl operation
You can also use the command `ioctl` to interact with this file operation. Two arguments are available for this command, the `cursor` command and the `reset` command: 

```bash
./tests ioctl cursor
~Cursor positions: r:3 | w:4.
```

```bash
./tests ioctl reset
~FIFO reset successful.
```
### sys/class interface
The driver provides sysfs interface to get the free and used space and also a graphical representation of the buffer. To see those, use those commands:
```bash
cat /sys/class/fifo/fifo[0-2]/free
2044
```
```bash
cat /sys/class/fifo/fifo[0-2]/used
4
```
```bash
cat /sys/class/fifo/fifo[0-2]/view
|h|e|y|!|@|...|@|@|@|@|@|
```

## License
- romainflcht