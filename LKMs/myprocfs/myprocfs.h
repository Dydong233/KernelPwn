#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/proc_fs.h>

#define PROC_NAME "myproc"

static struct proc_dir_entry *my_module_proc = NULL;

static struct file_operations my_module_fo = {
    .owner = THIS_MODULE,
};

