#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/init.h>  
#include <linux/delay.h>  
#include <asm/uaccess.h>  
#include <asm/irq.h>  
#include <asm/io.h>  
#include <linux/module.h>  
#include <linux/device.h>  
#include <linux/cdev.h>

#define RW_R8	  0
#define RW_R16	  1
#define RW_R32	  2
#define RW_W8	  3
#define RW_W16	  4
#define RW_W32	  5

static int major;
static struct cdev register_cdev;
static struct class *cls;

static 	int register_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	volatile unsigned char  *p8;
	volatile unsigned short *p16;
	volatile unsigned int 	*p32;

	volatile unsigned int buf[2];//用来接收arg，因为是long,所以需要两个int
	unsigned int addr;
	volatile unsigned int val;

	copy_from_user(buf, (const void __user *)arg, 12);
	addr = buf[0];
	val  = buf[1];
	printk("addr : 0x%x val : 0x%x\n",addr,val);

	p8  = (volatile unsigned char  *)ioremap(addr, 4);
	p16 = p8;
	p32 = p8;

	switch(cmd)
	{
		case RW_R8:
		{
			val = *p8;
			copy_to_user((void __user *)(arg + 4), &val, 4);
			break;
		}
		
		case RW_R16:
		{
			val = *p16;
			copy_to_user((void __user *)(arg + 4), &val, 4);
			break;
		}
		
		case RW_R32:
		{
			val = *p32;
			copy_to_user((void __user *)(arg + 4), &val, 4);
			break;
		}
		case RW_W8:
		{
			*p8 = val;
			break;
		}
		case RW_W16:
		{
			*p16 = val;
			break;
		}
		case RW_W32:
		{
			*p32 = val;
			break;
		}
	}
	iounmap(p8);
	return 0;
}

static struct file_operations register_fops = {
	.owner = THIS_MODULE,
	.ioctl = register_ioctl,
};

static int __init register_init(void)
{
	dev_t	dev_id;
	int ret;
	int err;
	
	/* 注册字符设备 */
	if(major)
	{
		dev_id = MKDEV(major, 0);
		ret = register_chrdev_region(major, 1, "registertools");
	}
	else
	{
		printk("alloc_chrdev_region start\n");
		ret = alloc_chrdev_region(&dev_id, 0, 2, "registertools");
		major = MAJOR(dev_id);
		printk("alloc_chrdev_region end\n");
	}

	if(ret < 0)
		return ret;
	
	cdev_init(&register_cdev,&register_fops);
	register_cdev.owner = THIS_MODULE;
	err = cdev_add(&register_cdev, dev_id, 1);
	if(err)
		printk(KERN_NOTICE"cdev error %d\n", err);
	
	/* 在类下面创建设备节点 */
	cls = class_create(THIS_MODULE, "registertools");
	class_device_create(cls, NULL, MKDEV(major,0), NULL, "registertools");
	
	return 0;
}

static void __exit register_exit(void)
{
	class_device_destroy(cls, MKDEV(major,0));
	cdev_del(&register_cdev);
	unregister_chrdev_region(MKDEV(major,0), 1);
}

module_init(register_init);
module_exit(register_exit);
MODULE_LICENSE("GPL");

