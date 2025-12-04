#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

// * _ DEFINES _________________________________________________________________

#define DRIVER_VERSION          "1.0"

/// @brief defines the compilation method.
/// - 1: Will be compiled as a module to insert into the kernel. 
/// - 0: Will be compiled for linux kernel integration. 
#define MODULE_COMPILATION      1

/// @brief defines the debug level.
/// - 2: All debug prints will be displayed. 
/// - 1: Only errors debug will be displayed. 
/// - 0: Nothing will be printed. 
#define DEBUG                   1


// Defines the major number used by the kernel. 
// /!\ WARNING: Edit only the one used when MODULE_COMPILATION is not defined. 
#ifdef MODULE_COMPILATION
    #define FIFO_MAJOR_NUMBER   0
#else
    #define FIFO_MAJOR_NUMBER   201
#endif


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

#endif 