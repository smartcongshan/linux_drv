#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

static const struct i2c_device_id at24cxx_id[] = {
	{ "at24c08", 0 },
	{ }
};

static int major;
static struct class *cls;
static struct i2c_client *at24cxx_client;

static ssize_t at24cxx_read (struct file *file, char __user *buf, size_t size, loff_t *offset)
{	
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	int ret;
	unsigned char addr,data;
	printk("copy from user begin\n");
	if( copy_from_user(&addr, buf, 1))
		return -1;
	printk("copy from user over\n");
	data = i2c_smbus_read_byte_data(at24cxx_client,addr);
	printk("data : 0x%x\n",data);
	if(copy_to_user(buf,&data,1))
		return -1;
	return 1;
}

static ssize_t at24cxx_write(struct file *file, const char __user *buf , size_t size, loff_t *offset)
{
	int ret;
	unsigned char key_buf[2];
	unsigned char addr,data;

	if(copy_from_user(key_buf, buf, 2))
		return -2;
	addr = key_buf[0];
	data = key_buf[1];
	
	printk("addr = 0x%02x, data = 0x%02x\n", addr, data);
	if(!i2c_smbus_write_byte_data(at24cxx_client,addr,data))
		return 2;
	else
		return -EIO;
}

static struct file_operations at24cxx_fops = {
	.owner = THIS_MODULE,
	.read  = at24cxx_read,
	.write = at24cxx_write,
};

static int at24cxx_probe(struct i2c_client *client,
				 const struct i2c_device_id *id){
	at24cxx_client = client;
	major = register_chrdev(0,"at24cxx",&at24cxx_fops);

	cls = class_create(THIS_MODULE, "at24cxx");

	device_create(cls, NULL, MKDEV(major, 0), NULL, "at24cxx"); 
		return 0;
}

static int at24cxx_remove(struct i2c_client *client)
{
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	unregister_chrdev(major, "at24cxx");
	
	return 0;
}

static int at24cxx_detect(struct i2c_client *client,
		       struct i2c_board_info *info)
{
	printk("at24cxx_detect : addr = 0x%x\n", client->addr);
	strlcpy(info->type, "at24c08", I2C_NAME_SIZE);
	return 0;
}

static const unsigned short addr_list[] = {0x50, I2C_CLIENT_END };

static struct i2c_driver at24cxx_driver = {
	.driver = {
		.name	= "at24c08",
		.owner  = THIS_MODULE,
	},
	.probe		= at24cxx_probe,
	.remove		= at24cxx_remove,
	.id_table	= at24cxx_id,
};
static int at24cxx_drv_init(void)
{
	i2c_add_driver(&at24cxx_driver);
	return 0;
}
static void at24cxx_drv_exit(void)
{
	i2c_del_driver(&at24cxx_driver);
}

//module_i2c_driver(at24cxx_driver);

module_init(at24cxx_drv_init);
module_exit(at24cxx_drv_exit);
MODULE_LICENSE("GPL");

