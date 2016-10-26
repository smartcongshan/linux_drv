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
	/* ����һ��fb_info�ռ� */
	lcd_info = framebuffer_alloc(0,NULL);
	/* ���� */
	/* ����fix(�̶�)����*/
	strcpy(lcd_info->fix.id,"lcds");
	lcd_info->fix.smem_len    = 320*240*32/8;
	lcd_info->fix.type        = FB_TYPE_PACKED_PIXELS;
	lcd_info->fix.visual      = FB_VISUAL_TRUECOLOR;
	lcd_info->fix.line_length = 240*4;
	/* ����var(�ɱ�)����*/
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

	/* ���ò������� */
	lcd_info->fbops				 = &lcds_ops;
	/* �������� */
	lcd_info->pseudo_palette     = pseudo_palette;   //��ɫ��
	//lcd_info->screen_base	//�Դ������ַ
	lcd_info->screen_size		 = 240*320*32/8;
	/* Ӳ����صĲ��� */
	//lcd_info->fix.smem_start = xxx;  /* �Դ��������ַ */
	/* ����GPIO��������LCD���� */
	gpbcon = ioremap(0x56000010,8);
	gpbdat = gpbcon + 1;
	gpccon = ioremap(0x56000020,4);
	gpdcon = ioremap(0x56000030,4);
	gpgcon = ioremap(0x56000060,4);


	*gpccon = 0xaaaaaaaa;   /* GPIO�ܽ�����VD[7:0],LCDVF[2:0],VM,VFRAME,VLINE,VCLK,LEND */
	*gpdcon = 0xaaaaaaaa;   /* GPIO�ܽ�����VD[23:8] */
	*gpgcon |= (0x3<<8);	  /* GPG4����LCD_PWREN */

	/* ����lcd������������ӳ���ַ */
	lcd_regs = ioremap(0x4D000000,sizeof(struct lcd_regs));
	/* ����lcdcon1
	 * [17:8]
	 * VCLK = HCLK / [(CLKVAL+1) x 2]
	 * 10MHz = 100 / [(CLKVAL+1) x 2]
	 * CLKVAL = 4;
	 * [6:5]
	 * 11ѡ��ΪTFT��Ļ
	 * [4:1]
	 * 1100:24λbpp(RGB)
	 */
	lcd_regs->lcdcon1 = (9<<8) | (3<<5) | (0x0d<<1);
	/*����lcdcon2 
	 *[31:24]:VBPD = 0
 	 *[23:14]:LINEVAL = 319,��ΪYһ��320��
 	 *[13:6] :VFPD = 4;
 	 *[5:0]  :VSPW = 9;
	 */
	lcd_regs->lcdcon2 = (0<<24) | (319<<14) | (4<<6) | (9<<0);
	/*����lcdcon3 
	 * [25:19] HBPD = 25
	 * [18:8]HOZVL  = 239
   	 * [7:0]HFPD = 0
	 */
	lcd_regs->lcdcon3 = (25<<19) | (239<<8) | (0<<0);
	/* ����lcdcon4 */
	lcd_regs->lcdcon4 = (13<<8) | 4;
	/*����lcdcon5 
	 *[10]:1 VCLK������������
	 *[9] :1 HSYNC��Ҫ��ת
	 *[8] :1 VSYNC��Ҫ��ת
	 *[6]:VDEN��ת
	 *[12]:0
	 *[1]:0
	 *[0]:0����λ����ʾ��ʾ��˳��
	 */
	lcd_regs->lcdcon5 = (0 <<12) | (1<<10) | (1<<9) | (1<<8) | (0<<1) | (0<<0) | (1 << 6);

	/* �����Դ� */
	lcd_info->screen_base = dma_alloc_writecombine(NULL, lcd_info->fix.smem_len,&(lcd_info->fix.smem_start), GFP_KERNEL);

	lcd_regs->lcdsaddr1 = (lcd_info->fix.smem_start>>1) & ~(3<<30);
	lcd_regs->lcdsaddr2 = ((lcd_info->fix.smem_start + lcd_info->fix.smem_len) >> 1) & 0x1fffff;
	lcd_regs->lcdsaddr3 = (0<<11) | (240*32/16);

	/* ʹ��LCD */
	lcd_regs->lcdcon1 |= (1<<0);//ʹ��LCD������
	lcd_regs->lcdcon5 |= (1<<3);//ʹ��LCD����
	
 	/* ע�� */
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