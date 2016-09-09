#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/plat-s3c24xx/ts.h>

#include <asm/arch/regs-adc.h>
#include <asm/arch/regs-gpio.h>

struct s3c_ts_regs {
	unsigned long adccon;
	unsigned long adctsc;
	unsigned long adcdly;
	unsigned long adcdat0;
	unsigned long adcdat1;
	unsigned long adcupdn;
};

static volatile struct s3c_ts_regs *s3c_ts_regs;
static struct input_dev *ts_dev; 
static struct timer_list timer_ts;

static void enter_wait_pen_down_mode(void)
{
	s3c_ts_regs->adctsc = 0xd3;
}
static void enter_wait_pen_up_mode(void)
{
	s3c_ts_regs->adctsc = 0x1d3;
}

static void measure_xy(void)
{
	s3c_ts_regs->adctsc = (1<<2) | (1<<3);
}

static void adc_start(void)
{
		s3c_ts_regs->adccon |= (1<<0);
}

static irqreturn_t pen_down_up_irq(int irq, void *dev_id)
{
	if(s3c_ts_regs->adcdat0 & (1<<15))
	{
		//printk("pen up\n");
		enter_wait_pen_down_mode();
	}
	else
	{
		measure_xy();
		adc_start(); 
	}
	return IRQ_HANDLED;
}

static int filter_ts(int *x,int *y)
{
#define ERR 10
	int x_eva,y_eva;
	int x_det,y_det;

	x_eva = (x[0] + x[1])/2;
	y_eva = (y[0] + y[1])/2;

	x_det = (x_eva > x[2]) ? (x_eva - x[2]) : (x[2] - x_eva);
	y_det = (y_eva > y[2]) ? (y_eva - y[2]) : (y[2] - y_eva);

	if((x_det > ERR) || (y_det > ERR))
	return 0;

	x_eva = (x[1] + x[2])/2;
	y_eva = (y[1] + y[2])/2;

	x_det = (x_eva > x[3]) ? (x_eva - x[3]) : (x[3] - x_eva);
	y_det = (y_eva > y[3]) ? (y_eva - y[3]) : (y[3] - y_eva);
	if((x_det > ERR) || (y_det > ERR))
	return 0;

	return 1;
}

static irqreturn_t adc_irq(int irq, void *dev_id)
{
	static int con = 0;
	int adcdat0,adcdat1;
	static int x[4],y[4];

	adcdat0 = s3c_ts_regs->adcdat0 & 0x3ff;
	adcdat1 = s3c_ts_regs->adcdat1 & 0x3ff;

	if(s3c_ts_regs->adcdat0 & (1<<15))//这代表在AD转换还没有完成的时候就已经松开了
	{
		con = 0;
		enter_wait_pen_down_mode();//丢弃当前值，重新等待按下
	}
	else
	{
		x[con] = adcdat0;
		y[con] = adcdat1;
		++con;
		if(con == 4)
		{
			con = 0; 
			/* 软件过滤 */
			if(filter_ts(x,y))
			{
				input_report_abs(ts_dev, ABS_X, (x[0]+x[1]+x[2]+x[3])/4);
 				input_report_abs(ts_dev, ABS_Y, (y[0]+y[1]+y[2]+y[3])/4);

 				input_report_key(ts_dev, BTN_TOUCH, 1);
 				input_report_abs(ts_dev, ABS_PRESSURE, 1);
 				input_sync(ts_dev);
				//printk("x:%d,y:%d\n",(x[0]+x[1]+x[2]+x[3])/4,(y[0]+y[1]+y[2]+y[3])/4);
			}
			enter_wait_pen_up_mode();
			mod_timer(&timer_ts,jiffies + HZ/100);//改变定时器的时间10ms
		}
		else
		{
			measure_xy();
			adc_start(); 
		}
	}
	return IRQ_HANDLED;
}

void timer_ts_function(unsigned long dat)
{
	if(s3c_ts_regs->adcdat0 & (1<<15))
	{
		enter_wait_pen_down_mode();
	}	
	else
	{
		measure_xy();
		adc_start(); 
	}
}

static int s3c_ts_init(void)
{
	struct clk	*clk;
	/* 分配空间 */
	ts_dev = input_allocate_device();
	/* 设置 */

	
	set_bit(EV_KEY,ts_dev->evbit); //表示产生哪类事件
	set_bit(EV_ABS,ts_dev->evbit); //触摸屏

	set_bit(BTN_TOUCH,ts_dev->keybit);
	input_set_abs_params(ts_dev, ABS_X, 0, 0x3FF, 0, 0);
	input_set_abs_params(ts_dev, ABS_Y, 0, 0x3FF, 0, 0);
	input_set_abs_params(ts_dev, ABS_PRESSURE, 0, 1, 0, 0);

	/* 注册 */
	input_register_device(ts_dev);
	/* 硬件相关的代码 */
	/* 使能clkcon[15] */
	clk = clk_get(NULL, "adc");
	clk_enable(clk);

	/* 对对应管脚进行ioremap */
	s3c_ts_regs = ioremap(0x58000000,sizeof(struct s3c_ts_regs));
	/* 设置ADC相应寄存器 */
	/* ADCCON 
	 * [14] : 1
	 * [13 : 6] : 49
	 * [0] : 0暂时为0
	 */
	s3c_ts_regs->adccon = (1<<14) | (49<<6);
	request_irq(IRQ_TC,  pen_down_up_irq, IRQF_SAMPLE_RANDOM,"ts_pen", NULL);
	request_irq(IRQ_ADC, adc_irq, 		  IRQF_SAMPLE_RANDOM,"adc", NULL);

	/* 让触摸屏检测到的时候要先延迟一段时间 */
	s3c_ts_regs->adcdly = 0xffff;

	/* 开启定时器来处理长按或者滑动 */

	init_timer(&timer_ts);
    timer_ts.function = timer_ts_function;
    add_timer(&timer_ts);
	
	enter_wait_pen_down_mode();

	return 0;

}
static void s3c_exit(void)
{
	free_irq(IRQ_TC,NULL);
	free_irq(IRQ_ADC,NULL);
	iounmap(s3c_ts_regs);
	input_unregister_device(ts_dev);
	input_free_device(ts_dev);
}

module_init(s3c_ts_init);
module_exit(s3c_exit);

MODULE_LICENSE("GPL");

