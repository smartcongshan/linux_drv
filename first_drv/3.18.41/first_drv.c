#include <linux/types.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <asm/uaccess.h>
#include <linux/io.h>

struct first_dev {
	struct cdev cdev;
};
static 	dev_t devno;
static volatile unsigned long *gpbcon = NULL;
static volatile unsigned long *gpbdat = NULL;

static int major;
static struct first_dev *first_dev;
static struct class *cls;


static int first_drv_open(struct inode *inode, struct file *file)
{
	file->private_data = first_dev;
	*gpbcon &= ~((0x3<<5*2)|(0x3<<6*2)|(0x3<<7*2)|(0x3<<8*2));
	*gpbcon |= ((0x1<<5*2)|(0x1<<6*2)|(0x1<<7*2)|(0x1<<8*2));
	return 0;
}

static int first_drv_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	struct first_dev *dev = file->private_data; 
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

static void first_drv_setup_cdev(struct first_dev *dev, int index)
{
	dev_t devno = MKDEV(major, index);
	int err;

	cdev_init(&dev->cdev,&first_drv_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add(&dev->cdev, devno, 1);
	if(err)
		printk(KERN_NOTICE"Error adding first_drv%d", err, index);	
}

static __init int first_drv_init(void)
{
	int ret;


	
	/* 注册设备以及获得设备号 */
	if(major)
	{
		ret = register_chrdev_region(devno, 1, "first_drv");
	}
	else
	{
		ret = alloc_chrdev_region(&devno, 0, 1, "first_drv");
		major = MAJOR(devno);
	}
	
	if(ret < 0)
		return ret;
	printk("major %d \n",major);
	first_dev = kzalloc(sizeof(struct first_dev), GFP_KERNEL);

	if(first_dev == NULL)
	{
		return -ENOMEM;
		goto fail_malloc;
	}
	printk("kzalloc over\n");
	first_drv_setup_cdev(first_dev, 0);
	/*
	printk("class create start\n");
	 /*利用mdev自动创建设备节点 
	cls = class_create(THIS_MODULE, "first_drv");
	if(cls)
	{
		return -ENOMEM;
		goto fail_class_create;
	}
	
	device_create(cls, NULL, devno, NULL, "first_drv");
	printk("device create end\n");
	*/
	/* 硬件相关的设置 */
	gpbcon = (volatile unsigned long *)ioremap(0x56000010, 16);
	gpbdat = gpbcon + 1;
	
	return 0;
fail_malloc:
	unregister_chrdev_region(devno, 1);
	return ret;
fail_class_create:
	unregister_chrdev_region(devno, 1);
	kfree(first_dev);
	return ret;
}
static __exit void first_drv_exit(void)
{
	class_destroy(cls);
	kfree(first_dev);
	cdev_del(&first_dev->cdev);
	unregister_chrdev_region(devno, 1);
}

module_init(first_drv_init);
module_exit(first_drv_exit);
MODULE_LICENSE("GPL");


