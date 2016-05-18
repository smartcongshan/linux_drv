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

static struct class *fourthdrv_class;
static struct class_device	*fourthdrv_class_dev;


volatile unsigned long *gpgcon;
volatile unsigned long *gpgdat;
/* 创建一个等待队列 */
static DECLARE_WAIT_QUEUE_HEAD(button_waitq);
wait_queue_head_t write_Monitor;
//struct semaphore sem;
/* 中断事件标志, 中断服务程序将它置1，fourth_drv_read将它清0 */
static volatile int ev_press = 0;
static struct pin_desc{
	unsigned int pin;
	unsigned int key_val;
};
static unsigned char key_val;
static struct pin_desc pins_desc[6] = {
	{S3C2410_GPG(0),  0x01},
	{S3C2410_GPG(3),  0x02},
	{S3C2410_GPG(5),  0x03},
	{S3C2410_GPG(6),  0x04},
	{S3C2410_GPG(7),  0x05},
	{S3C2410_GPG(11), 0x06},
};

static irqreturn_t gpio_threadhandler(int irq, void *dev_id)
{
	return IRQ_HANDLED;
}
static irqreturn_t buttons_irq(int irq, void *dev_id)
{
	struct pin_desc *pindesc = (struct pin_desc *)dev_id;
	unsigned int pinval;
	pinval = gpio_get_value(pindesc->pin);

	if(pinval)
	{
		key_val = 0x80 | pindesc->key_val;
	}
	else
	{
		key_val = pindesc->key_val;
	}
	//printk("irq = %d\n",irq);
	ev_press = 1;
	wake_up_interruptible(&button_waitq);
	return IRQ_HANDLED;
}


static int fourth_drv_open(struct inode *inode, struct file *file)
{
	request_threaded_irq(IRQ_EINT8,  buttons_irq, gpio_threadhandler, IRQ_TYPE_EDGE_BOTH,"K1", &pins_desc[0]);
	request_threaded_irq(IRQ_EINT11, buttons_irq, gpio_threadhandler, IRQ_TYPE_EDGE_BOTH,"K2", &pins_desc[1]);
	request_threaded_irq(IRQ_EINT13, buttons_irq, gpio_threadhandler, IRQ_TYPE_EDGE_BOTH,"K3", &pins_desc[2]);
	request_threaded_irq(IRQ_EINT14, buttons_irq, gpio_threadhandler, IRQ_TYPE_EDGE_BOTH,"K4", &pins_desc[3]);
	request_threaded_irq(IRQ_EINT15, buttons_irq, gpio_threadhandler, IRQ_TYPE_EDGE_BOTH,"K5", &pins_desc[4]);
	request_threaded_irq(IRQ_EINT19, buttons_irq, gpio_threadhandler, IRQ_TYPE_EDGE_BOTH,"K6", &pins_desc[5]);
	return 0;
}
ssize_t fourth_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	if (size != 1)
		return -EINVAL;
	/* 如果不按键那就休眠，如果ev_press是0就代表是休眠，如果是1就跳过这个函数 */
	wait_event_interruptible(button_waitq,ev_press);
	
	/* 如果按下按键就返回值 */	
	copy_to_user(buf, &key_val, 1);
	ev_press = 0;
	return 1;
}
int fourth_drv_close(struct inode *inode, struct file *file)
{
	free_irq(IRQ_EINT8,  &pins_desc[0]);
	free_irq(IRQ_EINT11, &pins_desc[1]);
	free_irq(IRQ_EINT13, &pins_desc[2]);
	free_irq(IRQ_EINT14, &pins_desc[3]);
	free_irq(IRQ_EINT15, &pins_desc[4]);
	free_irq(IRQ_EINT19, &pins_desc[5]);
	return 0;
}
unsigned int fourth_drv_poll(struct file *file, struct socket *sock,
			   poll_table *wait)
{
	unsigned int mask = 0;
	poll_wait(file, &button_waitq, wait);
	
	if (ev_press)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static struct file_operations fourth_drv_fops = {
    .owner   =   THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
    .open    =   fourth_drv_open,
    .read    = 	 fourth_drv_read,
    .release =   fourth_drv_close,
    .poll	 = 	 fourth_drv_poll,	
};
int major;
int fourth_init(void)
{
//	sema_init(&sem, 1);
	major = register_chrdev(0,"fourth_drv", &fourth_drv_fops);
	fourthdrv_class = class_create(THIS_MODULE, "fourth_drv");

	fourthdrv_class_dev = device_create(fourthdrv_class, NULL, MKDEV(major, 0), NULL, "buttons"); 

	//gpgcon = (volatile unsigned long *)ioremap(0x56000060, 16);
	//gpgdat = gpgcon + 1;
	
	return 0;
}
void fourth_exit(void)
{
	unregister_chrdev(major, "fourth_drv");
	device_unregister(fourthdrv_class_dev);
	class_destroy(fourthdrv_class);
	
	//iounmap(gpgcon);
}
module_init(fourth_init);/*因为内核不知道我们用的是哪个注册驱动程序的函数，所以我们要用这个函数定义一个结构体指针来指向这个函数*/
module_exit(fourth_exit);
MODULE_LICENSE("GPL");

