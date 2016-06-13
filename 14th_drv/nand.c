#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/clk.h>
 
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
 
#include <asm/io.h>
 
#include <asm/arch/regs-nand.h>
#include <asm/arch/nand.h>

struct s3c_nand_regs{
	unsigned long nfconf  ;
	unsigned long nfcont  ;
	unsigned long nfcmd   ;
	unsigned long nfaddr  ;
	unsigned long nfdata  ;
	unsigned long nfeccd0 ;
	unsigned long nfeccd1 ;
	unsigned long nfeccd  ;
	unsigned long nfstat  ;
	unsigned long nfestat0;
	unsigned long nfestat1;
	unsigned long nfmecc0 ;
	unsigned long nfmecc1 ;
	unsigned long nfsecc  ;
	unsigned long nfsblk  ;
	unsigned long nfeblk  ;
};

static struct nand_chip *s3c_nand;
static struct mtd_info *s3c_mtd;
static struct s3c_nand_regs *s3c_nand_regs;
static struct mtd_partition s3c_nand_partitions[] = {
	[0] = {
        .name   = "bootloader",
        .size   = 0x00040000,
		.offset	= 0,
	},
	[1] = {
        .name   = "params",
        .offset = MTDPART_OFS_APPEND,
        .size   = 0x00020000,
	},
	[2] = {
        .name   = "kernel",
        .offset = MTDPART_OFS_APPEND,
        .size   = 0x00200000,
	},
	[3] = {
        .name   = "root",
        .offset = MTDPART_OFS_APPEND,
        .size   = MTDPART_SIZ_FULL,
	}
};


static void s3c2440_select_chip(struct mtd_info *mtd, int chipnr)
{
	if(chipnr == -1) //不选中
	{
		s3c_nand_regs->nfcont |=  (1<<1);
	}
	else //选中芯片
	{
		s3c_nand_regs->nfcont &= ~(1<<1);
	}
}

static void s3c2440_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{

	if (ctrl & NAND_CLE)
	{
		/* 发命令 DATACMMD = CMD */
		s3c_nand_regs->nfcmd  = cmd;
	}
	else
	{
		/* 发地址 */
		s3c_nand_regs->nfaddr = cmd;
	}
}


static int s3c2440_dev_ready(struct mtd_info *mtd)
{
	return (s3c_nand_regs->nfstat & (1<<0));
}

static int nand_init(void)
{
	struct clk *clk;

	/* 分配一个nand_chip结构体 */
	s3c_nand = kzalloc(sizeof(struct nand_chip), GFP_KERNEL);

	s3c_nand_regs = ioremap(0x4E000000, sizeof(struct s3c_nand_regs));
	/* 设置nand_chip结构体 */
	/* 根据nand_scan函数来设置结构体 */
	/* nand_chip应该干的几件事: 选中、发命令、读命令、发地址、判断状态 */
	s3c_nand->select_chip = s3c2440_select_chip; //选中芯片
	s3c_nand->cmd_ctrl    = s3c2440_cmd_ctrl;
	s3c_nand->IO_ADDR_R	  = &s3c_nand_regs->nfdata;
	s3c_nand->IO_ADDR_W	  = &s3c_nand_regs->nfdata;
	s3c_nand->dev_ready   = s3c2440_dev_ready;
	s3c_nand->ecc.mode = NAND_ECC_SOFT;	/* enable ECC */
	
	/* 硬件相关的设置 */

	clk = clk_get(NULL, "nand");
	clk_enable(clk);

	/* TACLS = 0ns 所以TACLS = 0 */
	/* TWRPH0 = 12ns 所以TWRPH0 = 1 */
	/* TWRPH1 = 5ns� 訲WRPH1 = 0 */
#define TACLS	0
#define TWRPH0	1
#define TWRPH1	0
	s3c_nand_regs->nfconf = (TACLS << 12) | (TWRPH0 << 8) | (TWRPH1 << 4);

	/* nfcont
	 * bit[1] : 1取消片选
	 * bit[0] : 1使能nandflash
	 */
	 s3c_nand_regs->nfcont = (1<<1) | (1<<0);

	/* 使用nand_scan() */
	s3c_mtd = kzalloc(sizeof(struct mtd_info),GFP_KERNEL);
	s3c_mtd->priv = s3c_nand;
	s3c_mtd->owner = THIS_MODULE;

	nand_scan(s3c_mtd, 1);
	/* 写入分区 */
	add_mtd_partitions(s3c_mtd,s3c_nand_partitions,4);
	return 0;
}

static void nand_exit(void)
{
	kfree(s3c_mtd);
	iounmap(s3c_nand_regs);
	kfree(s3c_nand);
}

module_init(nand_init);
module_exit(nand_exit);
MODULE_LICENSE("GPL");

