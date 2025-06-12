#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/gfp.h>

#define PAGE_ORDER 1

struct page *page;
unsigned long int virt_addr;

static int __init hello_init(void)
{
    page = alloc_pages(GFP_KERNEL, PAGE_ORDER);
    printk("page frame no: %lx\n", page_to_pfn(page));

    virt_addr = (unsigned int)page_address(page);
    printk("virtual addr: %x\n",(unsigned int)page_address(page));

    return 0;
}

static void __exit hello_exit(void)
{
    free_pages(virt_addr,PAGE_ORDER);
    printk("successfully rmmod the module\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("Dydong");
MODULE_LICENSE("GPL");