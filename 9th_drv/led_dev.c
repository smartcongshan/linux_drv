#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/serial_s3c.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <mach/hardware.h>
#include <asm/irq.h>
#include <asm/mach-types.h>

#include <mach/regs-gpio.h>
#include <mach/regs-lcd.h>

#include <mach/fb.h>
#include <linux/platform_data/i2c-s3c2410.h>
 
#include <linux/fs.h>  
 
#include <linux/delay.h>  
#include <asm/uaccess.h>  
#include <asm/irq.h>  

#include <linux/module.h>  
#include <linux/device.h> 


static struct resource led_resource[] = {
 [0] = {
  .start = 0x56000010,
  .end  = 0x56000010 + 8 - 1,
  .flags = IORESOURCE_MEM
 },
 [1] = {
  .start = 5,
  .end   = 5,
  .flags = IORESOURCE_IRQ
 }
};
void led_release(struct device *dev)
{
	
}
static struct platform_device led_dev = {
 .name  = "ledss",
 .id  = -1,
 .num_resources = ARRAY_SIZE(led_resource),
 .resource = led_resource,
 .dev = 
 {
	.release = led_release,
 },
};


static int led_dev_init(void)
{
	platform_device_register(&led_dev);
	return 0;
}

static void led_dev_exit(void)
{
	platform_device_unregister(&led_dev);
}

module_init(led_dev_init);
module_exit(led_dev_exit);
MODULE_LICENSE("GPL");

