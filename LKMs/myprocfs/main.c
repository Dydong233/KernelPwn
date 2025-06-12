#include "myprocfs.h"

static int __init kernel_module_init(void)
{
    printk(KERN_INFO "[my_TestModule:] Module loaded. Start to register proc...\n");
    my_module_proc = proc_create(PROC_NAME, 0666, NULL, (const struct proc_ops *)&my_module_fo);
    return 0;
}

static void __exit kernel_module_exit(void)
{
    printk(KERN_INFO "[my_TestModule:] Start to clean up the module.\n");
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "[my_TestModule:] Module clean up complete. See you next time.\n");
}

module_init(kernel_module_init);
module_exit(kernel_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("dydong");