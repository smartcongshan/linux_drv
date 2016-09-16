#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/mutex.h>

static const struct i2c_device_id eeprom_id[] = {
	{ "at24c08", 0 },
	{ }
};


static int at24cxx_probe(struct i2c_client *client,
				 const struct i2c_device_id *id){
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static int at24cxx_remove(struct i2c_client *client)
{
	printk("%s %s %d\n", __FILE__, __FUNCTION__, __LINE__);
	return 0;
}

static struct i2c_driver at24cxx_driver = {
	.driver = {
		.name	= "at24c08",
		.owner = THIS_MODULE,
	},
	.probe		= at24cxx_probe,
	.remove		= at24cxx_remove,
	.id_table	= eeprom_id,
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

module_init(at24cxx_drv_init);
module_exit(at24cxx_drv_exit);
MODULE_LICENSE("GPL");

