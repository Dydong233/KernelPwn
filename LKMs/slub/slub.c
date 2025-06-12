#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

struct student{
	int id;
	int age;
	float score;
	void (*print_score)(int id);
	void (*print_age)(int id);
};

struct kmem_cache *stu_cache;
struct student *p1;
struct student *p2;

static int __init hello_init(void)
{
	stu_cache = kmem_cache_create("student",sizeof(struct student),0,SLAB_PANIC|SLAB_ACCOUNT,NULL);
	BUG_ON(stu_cache == NULL);
	printk("stu_cache = %x\n",(unsigned int)&stu_cache);
	
	p1 = kmem_cache_alloc(stu_cache,GFP_KERNEL);
	p2 = kmem_cache_alloc(stu_cache,GFP_KERNEL);
	if(p1&p2){
		printk("p1 object size = %d\n",sizeof(*p1));
		printk("p2 object size = %d\n",sizeof(*p2));
		printk("p1 object addr = %x\n",p1);
		printk("p2 object addr = %x\n",p2);
		printk("struct(student) = %d\n",sizeof(struct student));
	}
	return 0;
}

static void __exit hello_exit(void)
{
	kmem_cache_free(stu_cache,p1);
	kmem_cache_free(stu_cache,p2);
	kmem_cache_destroy(stu_cache);
	printk("Successfully destory the cache");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("Dydong");
MODULE_LICENSE("GPL");