#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <asm/arch/regs-gpio.h>
#include <asm/hardware.h>
#include <linux/poll.h>
#include <linux/dma-mapping.h>
#include <linux/cdev.h>

#define MEM_CPY_NO_DMA	0
#define MEM_CPY_DMA		1
#define MAX_NUM			1
#define BUF_SIZE		(512*1024) 	//需要复制的长度

#define DMA0_BASE_ADDR  0x4B000000
#define DMA1_BASE_ADDR  0x4B000040
#define DMA2_BASE_ADDR  0x4B000080
#define DMA3_BASE_ADDR  0x4B0000C0

static DECLARE_WAIT_QUEUE_HEAD(dma_waitq);

static int major = 0;
static struct cdev s3c_dma_cdev; 
static struct class *cls;
static struct class_device *class_dev;

static char *src;	//源地址的指针
static u32 srcphy;	//源地址的物理地址
static char *dst;	//目的地址指针
static u32 dstphy;	//目的地址的物理地址
static volatile int ev_dma;

struct s3c_dma_regs{
	unsigned int   disrc; 
	unsigned int   disrcc;
	unsigned int   didst;
	unsigned int   didstc;
	unsigned int   dcon;
	unsigned int   dstat;
	unsigned int   dcsrc;
	unsigned int   dcdst;
	unsigned int   dmasktrig; 
};

static struct s3c_dma_regs *dma_regs;

static int s3c_dma_ioctrl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	int i;
	
	memset(src,0xaa,BUF_SIZE);
	memset(dst,0xaa,BUF_SIZE);
	
	switch(cmd)
	{
		case MEM_CPY_NO_DMA:
		{
			for(i = 0;i < BUF_SIZE;i++)
				dst[i] = src[i];
			if(memcmp(src,dst,BUF_SIZE) == 0)
				printk("COPY_OK\n");
			else
				printk("COPY_Feild\n");
			break;
		}
		case MEM_CPY_DMA:
		{
			ev_dma = 0; 
			dma_regs->disrc  = srcphy;
			dma_regs->disrcc = (0<<1)|(0<<0);
			dma_regs->didst  = dstphy;
			dma_regs->didstc = (0<<2)|(0<<1)|(0<<0);
			dma_regs->dcon   = (1<<30)|(1<<29)|(0<<28)|(1<<27)|(0<<23)|(0<<20)|(BUF_SIZE<<0);

			dma_regs->dmasktrig = (1<<1)|(1<<0);

			/* 设置完DMA就要启动，启动之后为了不占用资源所以需要休眠 */
			wait_event_interruptible(dma_waitq,ev_dma);


			if(memcmp(src,dst,BUF_SIZE) == 0)
				printk("COPY_OK\n");
			else
				printk("COPY_Feild\n");
			break;
		}
	}
	return 0;
}

static struct file_operations s3c_dma_fops = {
	.owner  = THIS_MODULE,
	.ioctl = s3c_dma_ioctrl, 
};

static irqreturn_t s3c_dma_irq(int irq, void *devid)
{
	/* 唤醒 */
	ev_dma = 1; 
	wake_up_interruptible(&dma_waitq);
	return IRQ_HANDLED;
}

static int s3c_dma_init(void)
{

	if(request_irq(IRQ_DMA3,s3c_dma_irq,0,"s3c_dma",1))
	{
		printk("can't request_irq for DMA\n");
		return -EBUSY;
	}
	
	int rc;
	dev_t devid;
	/* 分配对应的源和目的的缓冲区 */
	src = dma_alloc_writecombine(NULL,BUF_SIZE,&srcphy,GFP_KERNEL);
	if(src == NULL)
	{
		printk("can't alloc buffer for src\n");
		return -ENOMEM;
	}
	dst = dma_alloc_writecombine(NULL,BUF_SIZE,&dstphy,GFP_KERNEL);
	if(dst == NULL)
	{
		free_irq(IRQ_DMA3,1);
		dma_free_writecombine(NULL, BUF_SIZE, src, &srcphy);
		printk("can't alloc buffer for dst\n");
		return -ENOMEM;
	}
#if 0
	if (major)
	{
		devid = MKDEV(major, 0);		
		rc = register_chrdev_region(devid, MAX_NUM, "s3c_dma");
	}
	else 
	{		
		rc = alloc_chrdev_region(&devid, 0, MAX_NUM, "s3c_dma");
		major = MAJOR(devid);	
	}
	cdev_init(&s3c_dma_cdev,&s3c_dma_fops);
	cdev_add(&s3c_dma_cdev,devid,MAX_NUM);
#else
	major = register_chrdev(0, "s3c_dma", &s3c_dma_fops);
#endif
	cls = class_create(THIS_MODULE,"s3c_dma");
	class_device_create(cls, NULL, MKDEV(major,0),NULL,"s3c_dma");

	dma_regs = ioremap(DMA3_BASE_ADDR, sizeof(struct s3c_dma_regs));
	
	return 0;
}
void s3c_dma_exit(void)
{
	iounmap(dma_regs);
	class_device_destroy(cls,MKDEV(major,0));
	class_destroy(cls);
	cdev_del(&s3c_dma_cdev);
	unregister_chrdev_region(MKDEV(major, 0), MAX_NUM);
	free_irq(IRQ_DMA3, 1);
}

module_init(s3c_dma_init);
module_exit(s3c_dma_exit);
MODULE_LICENSE("GPL");

