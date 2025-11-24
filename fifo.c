#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


MODULE_LICENSE("GPL"); 

static int __init fifo_init(void)
{
    printk(KERN_INFO "[FIFO] driver loaded successfully!\n"); 
    return 0; 
}


static void __exit fifo_exit(void)
{
    printk(KERN_INFO "[FIFO] driver unloaded successfully, goodbye!\n"); 
    return; 
}


module_init(fifo_init); 
module_exit(fifo_exit); 