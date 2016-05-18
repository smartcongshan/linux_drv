
#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/init.h>  
#include <linux/delay.h> 
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>  
#include <asm/irq.h> 
#include <asm/gpio.h>
#include <asm/io.h>  
#include <linux/module.h>  
#include <linux/device.h>  
#include <asm/irq.h>
#include <mach/gpio-samsung.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <net/sock.h>
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


static struct pin_desc{
	int irq;
	char *name;
	unsigned int pin;
	unsigned int key_val;
};

static struct pin_desc pins_desc[6] = {
	{IRQ_EINT8,  "K1", S3C2410_GPG(0),       KEY_L},
	{IRQ_EINT11, "K2", S3C2410_GPG(3),       KEY_S},
	{IRQ_EINT13, "K3", S3C2410_GPG(5),       KEY_C},
	{IRQ_EINT14, "K4", S3C2410_GPG(6),       KEY_D},
	{IRQ_EINT15, "K5", S3C2410_GPG(7),   KEY_ENTER},
	{IRQ_EINT19, "K6", S3C2410_GPG(11),  KEY_SPACE},
};

static struct input_dev *buttons_dev;
static struct timer_list buttons_timer;
static struct pin_desc *irq_pd;

static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	irq_pd = (struct pin_desc *)dev_id;
	mod_timer(&buttons_timer,jiffies + HZ/100);//改变定时器的时间
	return IRQ_RETVAL(IRQ_HANDLED);
}

void buttons_timer_function(unsigned long data)
{
	struct pin_desc *pindesc = (struct pin_desc *)irq_pd;
	unsigned int pinval;
	pinval = gpio_get_value(pindesc->pin);

	if(pinval)
	{
		input_event(buttons_dev,EV_KEY,pindesc->key_val,0);
		input_sync(buttons_dev);
	}
	else
	{
		input_event(buttons_dev,EV_KEY,pindesc->key_val,1);
		input_sync(buttons_dev);
	}
}

static int __init buttons_init(void)
{

	int i;
	/* 分配一个input结构体 */

	buttons_dev = input_allocate_device();
	
	/* 配置结构体 */

	__set_bit(EV_KEY,buttons_dev->evbit); //表示产生哪类事件
	__set_bit(EV_REP,buttons_dev->evbit);

	__set_bit(KEY_L,	buttons_dev->keybit);//表示按键是哪个
	__set_bit(KEY_S,	buttons_dev->keybit);
	__set_bit(KEY_C,	buttons_dev->keybit);
	__set_bit(KEY_D,	buttons_dev->keybit);
	__set_bit(KEY_ENTER,buttons_dev->keybit);
	__set_bit(KEY_SPACE,buttons_dev->keybit);
	
	/* 注册 */

	input_register_device(buttons_dev);
	
	/* 硬件相关的处理函数 */

	init_timer(&buttons_timer);
	buttons_timer.function = buttons_timer_function;
	add_timer(&buttons_timer);

	for(i = 0;i < 6;i++)
	{
		request_threaded_irq(pins_desc[i].irq,  buttons_irq, NULL, IRQ_TYPE_EDGE_BOTH,pins_desc[i].name, &pins_desc[i]);
	}
	
	return 0;
}
static void __exit buttons_exit(void)
{
	int i;
	for(i = 0;i < 4;i++)
	{
		free_irq(pins_desc[i].irq,&pins_desc[i]);
	}

	input_unregister_device(buttons_dev);
	input_free_device(buttons_dev);
}

module_init(buttons_init);
module_exit(buttons_exit);

MODULE_LICENSE("GPL");
