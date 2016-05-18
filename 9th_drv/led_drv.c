
#include <linux/module.h>

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <asm/uaccess.h>

static struct class *cls;
static struct class_device	*cls_dev;
static volatile unsigned long *gpbcon = NULL;
static volatile unsigned long *gpbdat = NULL;
static int major;
static int pin;

static int led_open(struct inode *inode, struct file *file)
{
	//printk("first_drv_open\n");
	*gpbcon &= ~(0x3<<pin*2);
	*gpbcon |=  (0x1<<pin*2);
	return 0;
}
static int led_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	//printk("first_drv_write\n");
	int val;
	copy_from_user(&val,buf,count);
	if(val == 1)
	{
		*gpbdat &= ~(1<<pin);
	}
	else
	{
		*gpbdat |= (1<<pin);
	}
	return 0;
}

static struct file_operations led_fops = {
    .owner  =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open   =   led_open,     
	.write	=	led_write,	   
};

static int led_probe(struct platform_device *pdev)
{

	struct resource *res;


	/* 进行ioremap */

	res    = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gpbcon = ioremap(res->start,res->end - res->start + 1);
	gpbdat = gpbcon + 1;

	res    = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	pin    = res->start;
	
	/* 注册设备以及创建设备节点 */
	major = register_chrdev(0,"ledss", &led_fops);


	cls= class_create(THIS_MODULE, "ledss");

	cls_dev = device_create(cls, NULL, MKDEV(major, 0), NULL, "ldz"); /* /dev/xyz */
	
	printk("ledss find!\n");
	return 0;
}

static int led_remove(struct platform_device *pdev)
{
	device_destroy(cls_dev,MKDEV(major, 0));
	class_destroy(cls);
	unregister_chrdev(major,"ledss");
	iounmap(gpbcon);
	printk("ledss removed\n");	
	return 0;
}

static struct platform_driver led_drv = {
	.probe		= led_probe,
	.remove		= led_remove,
	.driver		= {
		.name	= "ledss",
	}
};



static int led_drv_init(void)
{
	platform_driver_register(&led_drv);
	return 0;
}

static void led_drv_exit(void)
{
	platform_driver_unregister(&led_drv);

}

module_init(led_drv_init);
module_exit(led_drv_exit);
MODULE_LICENSE("GPL");

