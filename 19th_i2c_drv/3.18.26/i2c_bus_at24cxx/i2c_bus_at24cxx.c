#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/cpufreq.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>

#include <plat/gpio-core.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-cfg-helpers.h>

#include <mach/regs-gpio.h>

#include <asm/irq.h>

#include <linux/platform_data/i2c-s3c2410.h>

/* see s3c2410x user guide, v1.1, section 9 (p447) for more info */

#define S3C2410_IICCON			0x00
#define S3C2410_IICSTAT			0x04
#define S3C2410_IICADD			0x08
#define S3C2410_IICDS			0x0C
#define S3C2440_IICLC			0x10

#define S3C2410_IICCON_ACKEN		(1 << 7)
#define S3C2410_IICCON_TXDIV_16		(0 << 6)
#define S3C2410_IICCON_TXDIV_512	(1 << 6)
#define S3C2410_IICCON_IRQEN		(1 << 5)
#define S3C2410_IICCON_IRQPEND		(1 << 4)
#define S3C2410_IICCON_SCALE(x)		((x) & 0xf)
#define S3C2410_IICCON_SCALEMASK	(0xf)

#define S3C2410_IICSTAT_MASTER_RX	(2 << 6)
#define S3C2410_IICSTAT_MASTER_TX	(3 << 6)
#define S3C2410_IICSTAT_SLAVE_RX	(0 << 6)
#define S3C2410_IICSTAT_SLAVE_TX	(1 << 6)
#define S3C2410_IICSTAT_MODEMASK	(3 << 6)

#define S3C2410_IICSTAT_START		(1 << 5)
#define S3C2410_IICSTAT_BUSBUSY		(1 << 5)
#define S3C2410_IICSTAT_TXRXEN		(1 << 4)
#define S3C2410_IICSTAT_ARBITR		(1 << 3)
#define S3C2410_IICSTAT_ASSLAVE		(1 << 2)
#define S3C2410_IICSTAT_ADDR0		(1 << 1)
#define S3C2410_IICSTAT_LASTBIT		(1 << 0)

#define S3C2410_IICLC_SDA_DELAY0	(0 << 0)
#define S3C2410_IICLC_SDA_DELAY5	(1 << 0)
#define S3C2410_IICLC_SDA_DELAY10	(2 << 0)
#define S3C2410_IICLC_SDA_DELAY15	(3 << 0)
#define S3C2410_IICLC_SDA_DELAY_MASK	(3 << 0)

#define S3C2410_IICLC_FILTER_ON		(1 << 2)

/* Treat S3C2410 as baseline hardware, anything else is supported via quirks */
#define QUIRK_S3C2440		(1 << 0)
#define QUIRK_HDMIPHY		(1 << 1)
#define QUIRK_NO_GPIO		(1 << 2)
#define QUIRK_POLL		(1 << 3)

/* Max time to wait for bus to become idle after a xfer (in us) */
#define S3C2410_IDLE_TIMEOUT	5000

/* i2c controller state */
enum s3c24xx_i2c_state {
	STATE_IDLE,
	STATE_START,
	STATE_READ,
	STATE_WRITE,
	STATE_STOP
};

struct s3c2440_i2c_xfer_data{
	struct i2c_msg *msgs;
	int smg_num;
	int cur_msg;
	int cur_ptr;
	int state;
	int err;
	wait_queue_head_t wait;
};

struct s3c2440_i2c_regs{
	unsigned int iiccon;
	unsigned int iicstat;
	unsigned int iicadd;
	unsigned int iicds;
	unsigned int iiclc;
};

static struct s3c2440_i2c_regs *s3c2440_i2c_regs;
static struct s3c2440_i2c_xfer_data s3c2440_xfer_data;

static void i2c_s3c2440_start(void)
{
		s3c2440_xfer_data.state = STATE_START;
	
		if(s3c2440_xfer_data.msgs->flags & I2C_M_RD) /* 读 */
		{
			s3c2440_i2c_regs->iicds = s3c2440_xfer_data.msgs->addr << 1;
			s3c2440_i2c_regs->iicstat = 0xb0;	
		}
		else /* 写 */
		{
			s3c2440_i2c_regs->iicds= s3c2440_xfer_data.msgs->addr << 1;
			s3c2440_i2c_regs->iicstat = 0xf0;
		}

}

static void i2c_s3c2440_stop(int err)
{
	s3c2440_xfer_data.state = STATE_STOP;
	s3c2440_xfer_data.err   = err;

	if(s3c2440_xfer_data.msgs->flags & I2C_M_RD) /* 读 */
	{
			s3c2440_i2c_regs->iicstat = 0xd0;
			s3c2440_i2c_regs->iiccon  = 0xaf;
			ndelay(50);
	}
	else /* 写 */
	{
			s3c2440_i2c_regs->iicstat = 0x90;                
			s3c2440_i2c_regs->iiccon= 0xaf;
			ndelay(50);
	}

	/* 唤醒 */
	wake_up(&s3c2440_xfer_data.wait);
	
}

static int s3c24xx_bus_i2c_xfer(struct i2c_adapter *adap,
			struct i2c_msg *msgs, int num)
{
		unsigned long timeout;
				
		s3c2440_xfer_data.msgs    = msgs;
		s3c2440_xfer_data.smg_num = num;
		s3c2440_xfer_data.cur_ptr = 0;
		s3c2440_xfer_data.cur_msg = 0;

		i2c_s3c2440_start();

		/* 休眠 */

		timeout = wait_event_timeout(s3c2440_xfer_data.wait, (s3c2440_xfer_data.state == STATE_STOP), HZ * 5);
		if(timeout == 0)
			return -ETIMEDOUT;
		else
			return s3c2440_xfer_data.err;
}

static u32 s3c24xx_bus_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL |	I2C_FUNC_PROTOCOL_MANGLING;
}

static const struct i2c_algorithm at24cxx_algo = {
	.master_xfer		= s3c24xx_bus_i2c_xfer,
	.functionality		= s3c24xx_bus_i2c_func,
};

static struct i2c_adapter at24cxx_adapter = {
	.name			= "at24c08",
	.algo			= &at24cxx_algo,
	.owner 			= THIS_MODULE,
};

static void s3c2440_i2c_init(void)
{
	struct clk *clk;
	printk("clk start\n");
	clk = clk_get(NULL,"i2c");
	int ret = clk_prepare_enable(clk);
	if (ret) {
	pr_err("nand: clock failed to prepare+enable: %d\n", ret);
	clk_put(clk);
	//return ;
	}
	//IICSDA IICSCL
	s3c_gpio_cfgpin(S3C2410_GPE(14), S3C2410_GPE14_IICSCL);
	s3c_gpio_cfgpin(S3C2410_GPE(15), S3C2410_GPE15_IICSDA);
	/*
	 * Acknowledge generation    [7]   : 1使能应答
	 * Tx clock source selection [6]   : 0: IICCLK = fPCLK /16
	 * Tx/Rx Interrupt           [5]   : 1: Enable
	 * Transmit clock value      [3:0] : 0xf
	 */
	s3c2440_i2c_regs->iiccon = (1<<7)|(0<<6)|(1<<5)|(0xf);

	s3c2440_i2c_regs->iicadd  = 0x10;     // S3C24xx slave address = [7:1]
	s3c2440_i2c_regs->iicstat = 0x10;
}

static int islastmsg(void)
{
	return (s3c2440_xfer_data.cur_msg == s3c2440_xfer_data.smg_num - 1);
}

static int isendData(void)
{
	return (s3c2440_xfer_data.cur_ptr >= s3c2440_xfer_data.msgs->len);
}
static int islastdata(void)
{
	return (s3c2440_xfer_data.cur_ptr == (s3c2440_xfer_data.msgs->len - 1));
}

static irqreturn_t at24cxx_i2c_xfer_irq(int irq, void *dev_id)
{
	unsigned int iicst;

	iicst = s3c2440_i2c_regs->iicstat;

	if(iicst & 0x8)
	{printk("Bus arbitration failed\n");}

	switch(s3c2440_xfer_data.state)
	{
		case STATE_START:
		{
			if(iicst & S3C2410_IICSTAT_LASTBIT) //无应答
			{
				i2c_s3c2440_stop(-ENODEV);
				break;
			}

			if(islastmsg() && isendData())
			{
				i2c_s3c2440_stop(0);
				break;
			}

			if(s3c2440_xfer_data.msgs->flags & I2C_M_RD) /* 读 */
			{
				s3c2440_xfer_data.state = STATE_READ;
				goto next_read;
			}
			else
			{
				s3c2440_xfer_data.state = STATE_WRITE;
			}
		}
		case STATE_WRITE://写数据
		{
			if(!isendData()) //如果不是最后一个数据 
			{
				s3c2440_i2c_regs->iicds  = s3c2440_xfer_data.msgs->buf[s3c2440_xfer_data.cur_ptr++];

				ndelay(50);

				s3c2440_i2c_regs->iiccon = 0xaf;

				break;
			}
			else if(!islastmsg()) //如果数据全都发完了，但是不是最后一个消息，就重新开始发送开始信号
			{
				s3c2440_xfer_data.msgs++;
				s3c2440_xfer_data.cur_msg++;
				s3c2440_xfer_data.cur_ptr = 0;
				s3c2440_xfer_data.state = STATE_START;

				i2c_s3c2440_start();

				break;
			}
			else //是最后一个消息的最后一个数据
			{
				i2c_s3c2440_stop(0);
				break;
			}
		}
		case STATE_READ:
		{
			/* 如果程序到这的话那就表明数据已经发送过来了 */
			s3c2440_xfer_data.msgs->buf[s3c2440_xfer_data.cur_ptr++] = s3c2440_i2c_regs->iicds;
next_read:
			if(!isendData())
			{
				if(islastdata()) //如果即将读的数据是最后一个数据，就没有ACK信号
				{
					s3c2440_i2c_regs->iiccon = 0x2f; //无应答
				}
				else
				{
					s3c2440_i2c_regs->iiccon = 0xaf;	
				}
				break;
			}
			else if(!islastmsg()) //如果数据发完了，但是消息还没有发完，就开始发送下一个
			{
				s3c2440_xfer_data.msgs++;
				s3c2440_xfer_data.smg_num++;
				s3c2440_xfer_data.cur_ptr = 0;
				s3c2440_xfer_data.state = STATE_START;

				i2c_s3c2440_start();
				break;
			}
			else //如果数据和消息全都发完了就发送停止信号
			{
				i2c_s3c2440_stop(0);
				break;
			}
			break;
		}
		default: break;
	}

		/* 清中断 */
		s3c2440_i2c_regs->iiccon &= ~S3C2410_IICCON_IRQPEND;
		
		return IRQ_HANDLED;
}


static int i2c_bus_at24cxx_init(void)
{
	/* 硬件相关的设置 */
	s3c2440_i2c_regs = ioremap(0x54000000,sizeof(struct s3c2440_i2c_regs));
	s3c2440_i2c_init();
	/* 注册adapter */	
	request_irq(IRQ_IIC, at24cxx_i2c_xfer_irq, 0, "s3c2440-i2c", NULL);
	init_waitqueue_head(&s3c2440_xfer_data.wait);
	i2c_add_numbered_adapter(&at24cxx_adapter);
	return 0;
}
static void i2c_bus_at24cxx_exit(void)
{	
	iounmap(s3c2440_i2c_regs);
	free_irq(IRQ_IIC, NULL);
	i2c_del_adapter(&at24cxx_adapter);
}
module_init(i2c_bus_at24cxx_init);
module_exit(i2c_bus_at24cxx_exit);
MODULE_LICENSE("GPL");

