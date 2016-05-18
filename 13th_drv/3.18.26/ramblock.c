#include <linux/major.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/blkdev.h>
#include <linux/bitops.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/hdreg.h>


#include <asm/setup.h>
#include <asm/pgtable.h>


#define RAMBLOCK_SIZE (1024*1024)
#define RAMBLOCK_COUNT 8

static int major;
static unsigned char *ram_block_buffer;
struct gendisk *ram_block_disk;
static struct request_queue *ram_block_queue;
static DEFINE_SPINLOCK(ram_block_lock);

static int ramblock_getgeo(struct block_device *bdev, struct hd_geometry *geo)
{
	/* ���� = heads * sectors * 512 */
	geo->heads     = 2;//��ͷ
	geo->cylinders = 32;//�����ж��ٻ�
	geo->sectors   = RAMBLOCK_SIZE/32/2/512;//һ���ж��ٸ�����
	return 0;
}

static struct block_device_operations ram_block_fops = {
	.owner	= THIS_MODULE,
	.getgeo = ramblock_getgeo,
};

static void ram_block_request(struct request_queue * q)
{

	struct request *req;
	static int r_cnt,w_cnt;
	req = blk_fetch_request(q);
	while (req) {
		unsigned long start = blk_rq_pos(req) << 9;
		unsigned long len  = blk_rq_cur_bytes(req);
		int err = 0;

		if (start + len > RAMBLOCK_SIZE) {
			pr_err( "ramblock: bad access: block=%llu, "
			       "count=%u\n",
			       (unsigned long long)blk_rq_pos(req),
			       blk_rq_cur_sectors(req));
			err = -EIO;
			goto done;
		}
		
			unsigned long addr = start + ram_block_buffer;
			//unsigned long size = Z2RAM_CHUNKSIZE - addr;
			void *buffer = bio_data(req->bio);

			//if (len < size)
			//	size = len;
			//addr += z2ram_map[ start >> Z2RAM_CHUNKSHIFT ];
			if (rq_data_dir(req) == READ)
			{
				printk("read:%d",r_cnt);
				memcpy(buffer, (char *)addr, len);
			}
			else
			{
				printk("write:%d",w_cnt);
				memcpy((char *)addr, buffer, len);
			}
			//start += size;
			//len -= size;

	done:
		if (!__blk_end_request_cur(req, err))
			req = blk_fetch_request(q);
	}
}

static struct kobject *ram_block_find(dev_t dev, int *part, void *data)
{
	*part = 0;
	return get_disk(ram_block_disk);
}



static int ram_block_init(void)
{
	/* ����һ��gendisk�ṹ�� */
	ram_block_disk = alloc_disk(16); //���豸�ŵĸ���
	/* ���� */
	/* ����/����һ������ */
	ram_block_queue = blk_init_queue(ram_block_request, &ram_block_lock);
	ram_block_disk->queue = ram_block_queue;

	/* ����������Ϣ */
	major = register_blkdev(0, "ramblock");
	ram_block_disk->major = major;
	ram_block_disk->first_minor = 0;
	sprintf(ram_block_disk->disk_name, "ramblock");
	ram_block_disk->fops = &ram_block_fops;
	set_capacity(ram_block_disk, RAMBLOCK_SIZE / 512);
	/* Ӳ����صĲ��� */
	ram_block_buffer = kzalloc(RAMBLOCK_SIZE,GFP_KERNEL);
	/* ע�� */
	add_disk(ram_block_disk);
    blk_register_region(MKDEV(major, 0), RAMBLOCK_COUNT, THIS_MODULE,
			ram_block_find, NULL, NULL);

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

