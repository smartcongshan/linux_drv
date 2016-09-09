#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/div64.h>

#include <asm/mach/map.h>



struct lcd_regs {
	unsigned long	lcdcon1;
	unsigned long	lcdcon2;
	unsigned long	lcdcon3;
	unsigned long	lcdcon4;
	unsigned long	lcdcon5;
    unsigned long	lcdsaddr1;
    unsigned long	lcdsaddr2;
    unsigned long	lcdsaddr3;
    unsigned long	redlut;
    unsigned long	greenlut;
    unsigned long	bluelut;
    unsigned long	reserved[9];
    unsigned long	dithmode;
    unsigned long	tpal;
    unsigned long	lcdintpnd;
    unsigned long	lcdsrcpnd;
    unsigned long	lcdintmsk;
    unsigned long	lpcsel;
};

static volatile unsigned long *gpbcon;
static volatile unsigned long *gpbdat;
static volatile unsigned long *gpccon;
static volatile unsigned long *gpdcon;
static volatile unsigned long *gpgcon;
static struct lcd_regs *lcd_regs;
static struct fb_info *lcd_info;
static u32 pseudo_palette[16];

/* from pxafb.c */
static inline unsigned int chan_to_field(unsigned int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}
static int lcd_setcolreg(unsigned regno,
			       unsigned red, unsigned green, unsigned blue,
			       unsigned transp, struct fb_info *info)
{
	unsigned int val;
	
	if (regno > 16)
	return 1;

	val  = chan_to_field(red,   &info->var.red);
	val |= chan_to_field(green, &info->var.green);
	val |= chan_to_field(blue,  &info->var.blue);

	pseudo_palette[regno] = val;
	
	return 0;
}

static struct fb_ops lcds_ops = {
	.owner		= THIS_MODULE,
	.fb_setcolreg	= lcd_setcolreg,
	.fb_fillrect	= cfb_fillrect,
	.fb_copyarea	= cfb_copyarea,
	.fb_imageblit	= cfb_imageblit,
};

static int lcd_init(void)
{
	/* 分配一个fb_info空间 */
	lcd_info = framebuffer_alloc(0,NULL);
	/* 设置 */
	/* 设置fix(固定)参数*/
	strcpy(lcd_info->fix.id,"lcds");
	lcd_info->fix.smem_len    = 320*240*32/8;
	lcd_info->fix.type        = FB_TYPE_PACKED_PIXELS;
	lcd_info->fix.visual      = FB_VISUAL_TRUECOLOR;
	lcd_info->fix.line_length = 240*4;
	/* 设置var(可变)参数*/
	lcd_info->var.xres           = 240;
	lcd_info->var.yres 		     = 320;
	lcd_info->var.xres_virtual   = 240;
	lcd_info->var.yres_virtual   = 320;
	lcd_info->var.bits_per_pixel = 32;// RGB:888

	lcd_info->var.red.offset       = 16;  
    lcd_info->var.red.length       = 8;  
    
    lcd_info->var.blue.offset  	   = 8;  
    lcd_info->var.blue.length      = 8;  
    
    lcd_info->var.green.offset     = 0;  
    lcd_info->var.green.length     = 8; 
    
	lcd_info->var.activate       = FB_ACTIVATE_NOW;

	/* 设置操作函数 */
	lcd_info->fbops				 = &lcds_ops;
	/* 其它设置 */
	lcd_info->pseudo_palette     = pseudo_palette;   //调色板
	//lcd_info->screen_base	//显存虚拟地址
	lcd_info->screen_size		 = 240*320*32/8;
	/* 硬件相关的操作 */
	//lcd_info->fix.smem_start = xxx;  /* 显存的物理地址 */
	/* 配置GPIO引脚用于LCD控制 */
	gpbcon = ioremap(0x56000010,8);
	gpbdat = gpbcon + 1;
	gpccon = ioremap(0x56000020,4);
	gpdcon = ioremap(0x56000030,4);
	gpgcon = ioremap(0x56000060,4);


	*gpccon = 0xaaaaaaaa;   /* GPIO管脚用于VD[7:0],LCDVF[2:0],VM,VFRAME,VLINE,VCLK,LEND */
	*gpdcon = 0xaaaaaaaa;   /* GPIO管脚用于VD[23:8] */
	*gpgcon |= (0x3<<8);	  /* GPG4用作LCD_PWREN */

	/* 给出lcd控制器的所有映射地址 */
	lcd_regs = ioremap(0x4D000000,sizeof(struct lcd_regs));
	/* 配置lcdcon1
	 * [17:8]
	 * VCLK = HCLK / [(CLKVAL+1) x 2]
	 * 10MHz = 100 / [(CLKVAL+1) x 2]
	 * CLKVAL = 4;
	 * [6:5]
	 * 11选择为TFT屏幕
	 * [4:1]
	 * 1100:24位bpp(RGB)
	 */
	lcd_regs->lcdcon1 = (9<<8) | (3<<5) | (0x0d<<1);
	/*配置lcdcon2 
	 *[31:24]:VBPD = 0
 	 *[23:14]:LINEVAL = 319,因为Y一共320个
 	 *[13:6] :VFPD = 4;
 	 *[5:0]  :VSPW = 9;
	 */
	lcd_regs->lcdcon2 = (0<<24) | (319<<14) | (4<<6) | (9<<0);
	/*配置lcdcon3 
	 * [25:19] HBPD = 25
	 * [18:8]HOZVL  = 239
   	 * [7:0]HFPD = 0
	 */
	lcd_regs->lcdcon3 = (25<<19) | (239<<8) | (0<<0);
	/* 配置lcdcon4 */
	lcd_regs->lcdcon4 = (13<<8) | 4;
	/*配置lcdcon5 
	 *[10]:1 VCLK这里是上升沿
	 *[9] :1 HSYNC需要反转
	 *[8] :1 VSYNC需要反转
	 *[6]:VDEN反转
	 *[12]:0
	 *[1]:0
	 *[0]:0这三位来表示显示的顺序
	 */
	lcd_regs->lcdcon5 = (0 <<12) | (1<<10) | (1<<9) | (1<<8) | (0<<1) | (0<<0) | (1 << 6);

	/* 分配显存 */
	lcd_info->screen_base = dma_alloc_writecombine(NULL, lcd_info->fix.smem_len,&(lcd_info->fix.smem_start), GFP_KERNEL);

	lcd_regs->lcdsaddr1 = (lcd_info->fix.smem_start>>1) & ~(3<<30);
	lcd_regs->lcdsaddr2 = ((lcd_info->fix.smem_start + lcd_info->fix.smem_len) >> 1) & 0x1fffff;
	lcd_regs->lcdsaddr3 = (0<<11) | (240*32/16);

	/* 使能LCD */
	lcd_regs->lcdcon1 |= (1<<0);//使能LCD控制器
	lcd_regs->lcdcon5 |= (1<<3);//使能LCD本身
	
 	/* 注册 */
	register_framebuffer(lcd_info);
	return 0;
}

static void lcd_exit(void)
{
	unregister_framebuffer(lcd_info);
	
	lcd_regs->lcdcon1 &= ~(1<<0);
	lcd_regs->lcdcon5 &= ~(1<<3);
	
	dma_free_writecombine(NULL,lcd_info->fix.smem_len,lcd_info->screen_base,lcd_info->fix.smem_start);
	iounmap(lcd_regs);
	iounmap(gpccon);
	iounmap(gpdcon);
	iounmap(gpgcon);
	framebuffer_release(lcd_info);
}

module_init(lcd_init);
module_exit(lcd_exit);
MODULE_LICENSE("GPL");
