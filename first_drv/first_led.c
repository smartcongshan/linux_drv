#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/init.h>  
#include <linux/delay.h>  
#include <asm/uaccess.h>  
#include <asm/irq.h>  
#include <asm/io.h>  
#include <linux/module.h>  
#include <linux/device.h>  

volatile unsigned long *gpbcon = NULL;
volatile unsigned long *gpbdat = NULL;

static struct class *firstdrv_class;
static struct class_device	*firstdrv_class_dev;

static int first_drv_open(struct inode *inode, struct file *file)
{
	//printk("first_drv_open\n");
	*gpbcon &= ~((0x3<<5*2)|(0x3<<6*2)|(0x3<<7*2)|(0x3<<8*2));
	*gpbcon |= ((0x1<<5*2)|(0x1<<6*2)|(0x1<<7*2)|(0x1<<8*2));
	return 0;
}
static int first_drv_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	//printk("first_drv_write\n");
	int val;
	copy_from_user(&val,buf,count);
	if(val == 1)
	{
		*gpbdat &= ~((1<<5)|(1<<6)|(1<<7)|(1<<8));
	}
	else
	{
		*gpbdat |= ((1<<5)|(1<<6)|(1<<7)|(1<<8));
	}
	return 0;
}
static struct file_operations first_drv_fops = {
    .owner  =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open   =   first_drv_open,     
	.write	=	first_drv_write,	   
};
int major;
int first_init(void)
{

	major = register_chrdev(0,"first_drv", &first_drv_fops); //注册驱动程序

	firstdrv_class = class_create(THIS_MODULE, "first_drv");

	firstdrv_class_dev = device_create(firstdrv_class, NULL, MKDEV(major, 0), NULL, "xyz"); /* /dev/xyz */

	gpbcon = (volatile unsigned long *)ioremap(0x56000010, 16);
	gpbdat = gpbcon + 1;
	return 0;
}
void first_exit(void)
{
	unregister_chrdev(major, "first_drv");

	device_unregister(firstdrv_class_dev);
	class_destroy(firstdrv_class);
	iounmap(gpbcon);
}
module_init(first_init);/*因为内核不知道我们用的是哪个注册驱动程序的函数，所以我们要用这个函数定义一个结构体指针来指向这个函数*/
module_exit(first_exit);
MODULE_LICENSE("GPL");


