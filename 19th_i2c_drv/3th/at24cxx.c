#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x50, I2C_CLIENT_END };

static unsigned short force_addr[] = {ANY_I2C_BUS, 0x60, I2C_CLIENT_END};
static unsigned short * forces[] = {force_addr,NULL};

static struct i2c_driver at24cxx_driver;

static struct i2c_client_address_data addr_data = {
	.normal_i2c	= normal_addr,
	.probe		= ignore,	//省略
	.ignore		= ignore,
//	.forces		= forces,	//强制让设备认为是这个设备
};

static int at24cxx_detect(struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *new_client;
	
	
	printk("at24cxx_detect\n");

	new_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL);

	new_client->addr = address;
	new_client->adapter = adapter;
	new_client->driver = &at24cxx_driver;
	new_client->flags = 0;

	/* Fill in the remaining client fields */
	strcpy(new_client->name, "at24cxx");

	i2c_attach_client(new_client);
	
	return 0;
}
 
static int at24cxx_attach_adapter(struct i2c_adapter *adapter)
{
	return i2c_probe(adapter, &addr_data, at24cxx_detect);
}
static int at24cxx_detach_client(struct i2c_client *client)
{
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

