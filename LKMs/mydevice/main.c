#include <linux/slab.h>
#include "mydevice.h"

static int __init kernel_module_init(void)
{
    spin_lock_init(&spin);
    printk(KERN_INFO "[my_TestModule:] Module loaded. Start to register device...\n");
    major_num = register_chrdev(0,DEVICE_NAME,&my_device_fo);
    if(major_num<0){
        printk(KERN_INFO "[my_TestModule:] Failed to register a major number.\n");
        return 0;
    }
    printk(KERN_INFO "[my_TestModule:] Register complete, major number: %d\n", major_num);
    module_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(module_class))
    {
        unregister_chrdev(major_num, DEVICE_NAME);
        printk(KERN_INFO "[my_TestModule:] Failed to register class device!\n");
        return PTR_ERR(module_class);
    }
    printk(KERN_INFO "[my_TestModule:] Class device register complete.\n");
    module_device = device_create(module_class,NULL,MKDEV(major_num, 0), NULL, DEVICE_NAME);
    if (IS_ERR(module_device))
    {
        class_destroy(module_class);
        unregister_chrdev(major_num, DEVICE_NAME);
        printk(KERN_INFO "[my_TestModule:] Failed to create the device!\n");
        return PTR_ERR(module_device);
    }
    printk(KERN_INFO "[my_TestModule:] Module register complete.\n");
    return 0;
}

static void __exit kernel_module_exit(void)
{
    printk(KERN_INFO "[my_TestModule:] Start to clean up the module.\n");
    device_destroy(module_class, MKDEV(major_num, 0));
    class_destroy(module_class);
    unregister_chrdev(major_num, DEVICE_NAME);
    printk(KERN_INFO "[my_TestModule:] Module clean up complete. See you next time.\n");
}

static long my_module_ioctl(struct file * __file, unsigned int cmd, unsigned long param)
{
    long ret;
    spin_lock(&spin);
    ret = __internal_my_module_ioctl(__file, cmd, param);
    spin_unlock(&spin);
    return ret;
}

static long __internal_my_module_ioctl(struct file * __file, unsigned int cmd, unsigned long param)
{
    printk(KERN_INFO "[my_TestModule:] Received operation code: %d\n", cmd);
    switch (cmd)
    {
    case READ_ONLY:
        if(!buffer){
            printk(KERN_INFO "[my_TestModule:] Please reset the buffer at first!\n");
            return -1;
        }
        printk(KERN_INFO "[my_TestModule:] Module operation mode reset to READ_ONLY.\n");
        my_module_mode = READ_ONLY;
        break;
    case ALLOW_WRITE:
        if(!buffer){
            printk(KERN_INFO "[my_TestModule:] Please reset the buffer at first!\n");
            return -1;
        }
        printk(KERN_INFO "[my_TestModule:] Module operation mode reset to ALLOW_WRITE.\n");
        my_module_mode = ALLOW_WRITE;
        break;
    case BUFFER_RESET:
        if(!buffer){
            buffer = kmalloc(0x500, GFP_ATOMIC);
            if (buffer == NULL)
            {
                printk(KERN_INFO "[my_TestModule:] Unable to initialize the buffer. Kernel malloc error.\n");
                my_module_mode = NOT_INIT;
                return -1;
            }
        }
        printk(KERN_INFO "[my_TestModule:] Buffer reset. Module operation mode reset to READ_ONLY.\n");
        memset(buffer, 0, 0x500);
        my_module_mode = READ_ONLY;
        break;
    case NOT_INIT:
        printk(KERN_INFO "[my_TestModule:] Module operation mode reset to NOT_INIT.\n");
        my_module_mode = NOT_INIT;
        if(buffer){
            kfree(buffer);
            buffer = NULL;
        }
        break;
    default:
        printk(KERN_INFO "[my_TestModule:] Invalid operation code.\n");
        return -1;
    }
    return 0;
}

static int my_module_open(struct inode * __inode, struct file * __file)
{
    spin_lock(&spin);
    if(buffer == NULL)
    {
        buffer = kmalloc(0x500, GFP_ATOMIC);
        if(buffer == NULL)
        {
            printk(KERN_INFO "[my_TestModule:] Unable to initialize the buffer. Kernel malloc error.\n");
            my_module_mode = NOT_INIT;
            return -1;
        }
        memset(buffer,0,sizeof(buffer));
        my_module_mode = READ_ONLY;
        printk(KERN_INFO "[my_TestModule:] Device open, buffer initialized successfully.\n");
    }
    else    printk(KERN_INFO "[my_TestModule:]Warning: reopen the device may cause unexpected error in kernel.\n");
    spin_unlock(&spin);
    return 0;
}

static ssize_t my_module_read(struct file * __file, char __user * user_buf, size_t size, loff_t * __loff)
{
    const char * const buf = (char*)buffer;
    int count;
    spin_lock(&spin);
    if (my_module_mode == NOT_INIT)
    {
        printk(KERN_INFO "[my_TestModule:] Buffer not initialized yet.\n");
        return -1;
    }
    count = copy_to_user(user_buf,buf,size > 0x500 ? 0x500:size);
    spin_unlock(&spin);
    return count;
}

static ssize_t my_module_write(struct file * __file, const char __user * user_buf, size_t size, loff_t * __loff)
{
    char * const buf = (char*)buffer;
    int count;
    spin_lock(&spin);
    if (my_module_mode == NOT_INIT)
    {
        printk(KERN_INFO "[my_TestModule:] Buffer not initialized yet.\n");
        count = -1;
    }
    else if(my_module_mode == READ_ONLY)
    {
        printk(KERN_INFO "[my_TestModule:] Unable to write under mode READ_ONLY.\n");
        count = -1;
    }
    else
        count = copy_from_user(buf, user_buf, size > 0x500 ? 0x500 : size);
    spin_unlock(&spin);
    return count;
}

static int my_module_release(struct inode * __inode, struct file * __file)
{
    spin_lock(&spin);
    if (buffer)
    {
        kfree(buffer);
        buffer = NULL;
    }
    printk(KERN_INFO "[my_TestModule:] Device closed.\n");
    spin_unlock(&spin);
    return 0;
}

module_init(kernel_module_init);
module_exit(kernel_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("dydong");