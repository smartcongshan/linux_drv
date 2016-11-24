#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <linux/types.h>
#include <linux/cdev.h>

#define MAX_NUM	2

static int major;
static struct class *cls;
static struct class_device	*class_dev;
static struct cdev hello_cdev;  

static int hello_open(struct inode *inode, struct file *file)
{
	printk("hello world %s\n",file);
	return 0;
}

static const struct file_operations hello_fileops = {
	.owner   = THIS_MODULE,
	.open    = hello_open,
};

static int hello_init(void)
{
	int rc;
	dev_t devid;

	if (major) {
		devid = MKDEV(major, 0);
		rc = register_chrdev_region(devid, MAX_NUM, "hello");
	} else {
		rc = alloc_chrdev_region(&devid, 0, MAX_NUM, "hello");
		major = MAJOR(devid);
	}
	if (rc < 0) {
		return -ENODEV;
	}

	cdev_init(&hello_cdev, &hello_fileops);
	cdev_add(&hello_cdev, devid, MAX_NUM);

	cls = class_create(THIS_MODULE, "hello");

	device_create(cls, NULL, MKDEV(major, 0), NULL, "hello1"); 
	device_create(cls, NULL, MKDEV(major, 1), NULL, "hello2"); 
	device_create(cls, NULL, MKDEV(major, 2), NULL, "hello3"); 
	

	return 0;
}
static void hello_exit(void)
{
	device_destroy(cls,MKDEV(major, 0));
	device_destroy(cls,MKDEV(major, 1));
	device_destroy(cls,MKDEV(major, 2));
	class_destroy(cls);

	cdev_del(&hello_cdev);
	unregister_chrdev_region(MKDEV(major, 0), MAX_NUM);
}

module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");

