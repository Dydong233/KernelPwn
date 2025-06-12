#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>

#define DEVICE_NAME "mydevice"
#define CLASS_NAME "mymodule"
#define NOT_INIT 0xffffffff
#define READ_ONLY 0x1000
#define ALLOW_WRITE 0x1001
#define BUFFER_RESET 0x1002

static int major_num = NULL;
static int my_module_mode = READ_ONLY;
static struct class * module_class = NULL;
static struct device * module_device = NULL;
static void * buffer = NULL;
static spinlock_t spin;

static int __init kernel_module_init(void);
static void __exit kernel_module_exit(void);
static int my_module_open(struct inode *, struct file *);
static ssize_t my_module_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t my_module_write(struct file *, const char __user *, size_t, loff_t *);
static int my_module_release(struct inode *, struct file *);
static long my_module_ioctl(struct file *, unsigned int, unsigned long);
static long __internal_my_module_ioctl(struct file * __file, unsigned int cmd, unsigned long param);


static struct file_operations my_device_fo = {
    .owner = THIS_MODULE,
    .open = my_module_open,
    .read = my_module_read,
    .write = my_module_write,
    .release = my_module_release,
    .unlocked_ioctl = my_module_ioctl,
};