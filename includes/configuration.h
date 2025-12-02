#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

// * _ DEFINES _________________________________________________________________


#define MODULE_COMPILATION

/// @brief defines the debug level.
/// - 2: All debug prints will be displayed. 
/// - 1: Only errors debug will be displayed. 
/// - 0: Nothing will be printed. 
#define DEBUG                   2

#ifdef MODULE_COMPILATION
    #define FIFO_MAJOR_NUMBER   0
#else
    #define FIFO_MAJOR_NUMBER   201
#endif

#define FIFO_DEV_COUNT          3
#define FIFO_BUFFER_SIZE        2048

#define ELT_CLASS_COUNT         10

#endif 