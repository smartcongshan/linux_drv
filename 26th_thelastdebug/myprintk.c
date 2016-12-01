#include <linux/kernel.h>  
#include <linux/fs.h>  
#include <linux/init.h>  
#include <linux/delay.h>  
#include <asm/uaccess.h>  
#include <asm/irq.h>  
#include <asm/io.h>  
#include <linux/module.h>  
#include <linux/device.h>  
#include <linux/proc_fs.h>

#define MAX_SIZE	(1024 * 1024)

struct proc_dir_entry *my_vmcore = NULL;
static char *mylog_buf;
static char tmp_buf[1024];
static int mylog_w = 0,mylog_r = 0;
static int mylog_r_for_read = 0;
static DECLARE_WAIT_QUEUE_HEAD(mymsg_waitq);

/* �жϻ��λ������Ƿ�Ϊ�� */
static int is_mylog_empty(void)
{
	return (mylog_w == mylog_r);
}
	
/* �жϻ��λ������Ƿ����� */	
static int is_mylog_full(void)
{
	return ((mylog_w + 1)%MAX_SIZE == mylog_r);
}

/* �����ݷ��뵽���λ������� */
static void mylog_putc(char c)
{
	if(is_mylog_full())	{
		mylog_r = (mylog_r + 1)%MAX_SIZE;//������˾Ͷ���һ������
		if((mylog_r_for_read + 1)%MAX_SIZE == mylog_r)
			mylog_r_for_read = mylog_r;
	}
	
	mylog_buf[mylog_w] = c;
	mylog_w = (mylog_w + 1)%MAX_SIZE;

	 wake_up_interruptible(&mymsg_waitq);  
	
}

/* �����ݴӻ��λ������ж����� */
static int mylog_getc(char *c)
{
	//���Ϊ�վͲ���Ҫ����
	if(is_mylog_empty())
		return 0;
	*c = mylog_buf[mylog_r];
	mylog_r = (mylog_r + 1)%MAX_SIZE;
	return 1;
}

static int is_mylog_empty_for_read(void)
{
	return (mylog_w == mylog_r_for_read);
}

static int mylog_getc_for_read(char *c)
{
	if(is_mylog_empty_for_read())
		return 0;
	*c = mylog_buf[mylog_r_for_read];
	mylog_r_for_read = (mylog_r_for_read + 1)%MAX_SIZE;
	return 1;
}

int myprintk(const char *fmt, ...)
{
	va_list args;
	int i;
	int j;

	va_start(args, fmt);
	i=vsnprintf(tmp_buf, INT_MAX, fmt, args);
	va_end(args);

	for(j = 0;j < i;j++)
		mylog_putc(tmp_buf[j]);
	return i;
}

static ssize_t mymsg_read(struct file *file, char __user *buf,
			 size_t count, loff_t *ppos)
{
	int error = 0;
	int i = 0;
	char c;
	
	if ((file->f_flags & O_NONBLOCK) && is_mylog_empty_for_read())
		return -EAGAIN;		
	
	error = wait_event_interruptible(mymsg_waitq,!is_mylog_empty_for_read());

		while (!error && mylog_getc_for_read(&c) && i < count) {
			/* ������copy_to_user */
			error = __put_user(c,buf);
			i++;
			buf++;
			}

	if(!error)
		error = i;
	return error;
}

static int mylog_open(struct inode *inode, struct file *file)
{
	mylog_r_for_read = mylog_r;
	return 0;
}

const struct file_operations my_proc_vmcore_operations = {
	.read = mymsg_read,
	.open = mylog_open,
};

static __init int myprintk_init(void)
{

	mylog_buf = kmalloc(MAX_SIZE, GFP_KERNEL);
	if(!mylog_buf)
		return -ENOMEM;
	
	my_vmcore  = create_proc_entry("mymsg", S_IRUSR, &proc_root);
	if (my_vmcore )
		my_vmcore ->proc_fops = &my_proc_vmcore_operations;
	return 0;
}

static __exit void myprintk_exit(void)
{
	remove_proc_entry("mymsg",&proc_root);
}

module_init(myprintk_init);
module_exit(myprintk_exit);

EXPORT_SYMBOL(myprintk);

MODULE_LICENSE("GPL");

