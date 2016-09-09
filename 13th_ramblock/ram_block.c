#include <linux/module.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/timer.h>
#include <linux/genhd.h>
#include <linux/hdreg.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/blkdev.h>
#include <linux/blkpg.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/dma.h>

#define RAMBLOCK_SIZE (1024*1024)

static int major;
static unsigned char *ram_block_buffer;
struct gendisk *ram_block_disk;
static struct request_queue *ram_block_queue;
static DEFINE_SPINLOCK(ram_block_lock);

static struct block_device_operations ram_block_fops = {
	.owner	= THIS_MODULE,
};

static void ram_block_request(request_queue_t * q)
{
	struct request *req;
	static int cnt = 0;
	printk("lalala%d\n",cnt++);
	while ((req = elv_next_request(q)) != NULL) {
			/* 数据传输三要素 : 源 目的 长度 */
			/* 源 */
			unsigned long offset = req->sector * 512; //扇区

			/* 目的 */
			//req->buffer
			/* 长度 */
			unsigned long len    = req->current_nr_sectors * 512;//当前扇区的数量也就是长度
			if (rq_data_dir(req) == READ)
				memcpy(req->buffer, ram_block_buffer + offset, len);
			else
				memcpy(ram_block_buffer + offset, req->buffer, len);
		
		
			end_request(req, 1);   //0失败 1成功
	}
}

static int ram_block_init(void)
{
	/* 分配一个gendisk结构体 */
	ram_block_disk = alloc_disk(16); //次设备号的个数
	/* 设置 */
	/* 分配/设置一个队列 */
	ram_block_queue = blk_init_queue(ram_block_request, &ram_block_lock);
	ram_block_disk->queue = ram_block_queue;

	/* 设置其他信息 */
	major = register_blkdev(0, "ramblock");
	ram_block_disk->major = major;
	ram_block_disk->first_minor = 0;
	sprintf(ram_block_disk->disk_name, "ramblock");
	ram_block_disk->fops = &ram_block_fops;
	set_capacity(ram_block_disk, RAMBLOCK_SIZE / 512);
	/* 硬件相关的操作 */
	ram_block_buffer = kzalloc(RAMBLOCK_SIZE,GFP_KERNEL);
	/* 注册 */
	add_disk(ram_block_disk);
	return 0;
}

static void ram_block_exit(void)
{
	unregister_blkdev(major,"ramblock");
	del_gendisk(ram_block_disk);
	put_disk(ram_block_disk);
    blk_cleanup_queue(ram_block_queue);
	kfree(ram_block_disk);
}

module_init(ram_block_init);
module_exit(ram_block_exit);
MODULE_LICENSE("GPL");


