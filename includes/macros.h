#ifndef _MACROS_H_
#define _MACROS_H_


// * _ MACROS __________________________________________________________________

#if defined(DEBUG) && DEBUG == 2
    #define ERR_DEBUG(...)  printk(KERN_ERR     __VA_ARGS__)
    #define WARN_DEBUG(...) printk(KERN_WARNING __VA_ARGS__)
    #define NOTE_DEBUG(...) printk(KERN_NOTICE  __VA_ARGS__)
    #define INFO_DEBUG(...) printk(KERN_INFO    __VA_ARGS__)

#elif defined(DEBUG) && DEBUG == 1
    #define ERR_DEBUG(...)  printk(KERN_ERR     __VA_ARGS__)
    #define WARN_DEBUG(...) printk(KERN_WARNING __VA_ARGS__)
    #define NOTE_DEBUG(...) 
    #define INFO_DEBUG(...) 
#else
    #define ERR_DEBUG(...) 
    #define WARN_DEBUG(...) 
    #define NOTE_DEBUG(...) 
    #define INFO_DEBUG(...) 
#endif


#endif