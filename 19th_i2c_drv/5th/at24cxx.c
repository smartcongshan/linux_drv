#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <asm/uaccess.h>


static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x50, I2C_CLIENT_END };

static unsigned short force_addr[] = {ANY_I2C_BUS, 0x60, I2C_CLIENT_END};
static unsigned short * forces[] = {force_addr,NULL};
struct i2c_client *at24cxx_client;
static struct i2c_driver at24cxx_driver;

static struct class *cls;
static struct class_device	*class_dev;

static struct i2c_client_address_data addr_data = {
	.normal_i2c	= normal_addr,
	.probe		= ignore,	//省略
	.ignore		= ignore,
//	.forces		= forces,	//强制让设备认为是这个设备
};

static int major;

static ssize_t at24cxx_read(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	struct i2c_msg msg[2];
	int ret;
	unsigned char address;
	unsigned char data;
	printk("at24cxx read begin\n");
	/*
   	 * val[0] = address
	 */
	if(size != 1)
		return -EINVAL;
	copy_from_user(&address,buf,1);
	printk("copy_from_user final\n");
	/* 写地址 */
	/* 三要素 : 源，目的，长度  */
	msg[0].addr  = at24cxx_client->addr; //目的
	msg[0].buf   = &address;					 //源
	msg[0].flags = 0;					 //读
	msg[0].len   = 1;
	/* 读取数据 */
	msg[0].addr  = at24cxx_client->addr; //源
	msg[0].buf   = &data;					 //目的
	msg[0].flags = I2C_M_RD;					 //读
	msg[0].len   = 1;
	
	ret = i2c_transfer(at24cxx_client->adapter,msg,2);
	printk("i2c_transfer final\n");
	if(ret == 2)
	{
		printk("copy_to_user begin\n");
		copy_to_user(buf, &data, 1);
		printk("copy_ti_user final\n");
		return 1;
	}
	else
		return -EIO;
}

static ssize_t at24cxx_write(struct file *file, char __user *buf, size_t size, loff_t *offset)
{
	struct i2c_msg msg[1];
	int ret;
	/*
   	 * val[0] = address
	 * val[1] = data
	 */
	char val[2];
	if(size != 2)
		return -EINVAL;
	copy_from_user(val,buf,2);
	/* 三要素 : 源，目的，长度  */
	msg[0].addr  = at24cxx_client->addr; //目的
	msg[0].buf   = val;					 //地址和数据 源
	msg[0].flags = 0;					 //写
	msg[0].len   = 2;

	ret = i2c_transfer(at24cxx_client->adapter,msg,1);
	if(ret == 1)
		return 2;
	else
		return -EIO;
}
	

static struct file_operations at24cxx_fops = {
    .owner = THIS_MODULE,
	.read  = at24cxx_read,
	.write = at24cxx_write,
};

static int at24cxx_detect(struct i2c_adapter *adapter, int address, int kind)
{
	
		
	printk("at24cxx_detect\n");

	at24cxx_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);

	at24cxx_client->addr = address;
	at24cxx_client->adapter = adapter;
	at24cxx_client->driver = &at24cxx_driver;


	/* Fill in the remaining client fields */
	strcpy(at24cxx_client->name, "at24cxx");

	i2c_attach_client(at24cxx_client);

	major = register_chrdev(0, "at24cxx", &at24cxx_fops);
	cls = class_create(THIS_MODULE, "at24cxx");

	class_device_create(cls, NULL, MKDEV(major, 0), NULL, "at24cxx"); 

	
	
	return 0;
}
 
static int at24cxx_attach_adapter(struct i2c_adapter *adapter)
{
	return i2c_probe(adapter, &addr_data, at24cxx_detect);
}
static int at24cxx_detach_client(struct i2c_client *client)
{

	i2c_detach_client(client);
	kfree(i2c_get_clientdata(client));
	printk("at24cxx_detach_client\n");
	return 0;
}

static struct i2c_driver at24cxx_driver = {
	.driver = {
		.name	= "at24cxx",
	},
	.id		= I2C_DRIVERID_EEPROM,
	.attach_adapter	= at24cxx_attach_adapter,
	.detach_client	= at24cxx_detach_client,
};


static int at24cxx_init(void)
{
	i2c_add_driver(&at24cxx_driver);	
	return 0;
}
static void at24cxx_exit(void)
{
	i2c_del_driver(&at24cxx_driver);
}
module_init(at24cxx_init);
module_exit(at24cxx_exit);
MODULE_LICENSE("GPL");

