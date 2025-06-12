#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init kernel_module_init(void)
{
    printk("<1>Hello the Linux kernel world!\n");
    return 0;
}

static void __exit kernel_module_exit(void)
{
    printk("<1>Good bye the Linux kernel world! See you again!\n");
}

module_init(kernel_module_init);
module_exit(kernel_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("arttnba3");
