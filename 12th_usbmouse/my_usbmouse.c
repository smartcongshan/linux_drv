#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

static struct input_dev *u_dev;
static char *usb_buf;
static dma_addr_t usb_adr_phy;
static struct urb *usb_urb;
static int len;

static struct usb_device_id my_usb_mouse_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};

static void my_usb_mouse_irq(struct urb *urb)
{	
	static unsigned char pre_val;
	#if 0
	int i;
	for(i = 0;i < len;i++)
	{
		printk("%x",usb_buf[i]);
	}
	printk("\n");
	#endif
	/*
      *bit[0]:鼠标左键
      *bit[1]:鼠标右键
      *bit[2]:鼠标中键
	 */
	if((pre_val & (1<<0)) != (usb_buf[0] & (1<<0)))
	{
		input_event(u_dev,EV_KEY,KEY_L,(usb_buf[0] & (1<<0)) ? 1 : 0);
		input_sync(u_dev);
	}
	if((pre_val & (1<<1)) != (usb_buf[0] & (1<<1)))
	{
		input_event(u_dev,EV_KEY,KEY_S,(usb_buf[0] & (1<<1)) ? 1 : 0);
		input_sync(u_dev);
	}
	if((pre_val & (1<<2)) != (usb_buf[0] & (1<<2)))
	{
		input_event(u_dev,EV_KEY,KEY_ENTER,(usb_buf[0] & (1<<2)) ? 1 : 0);
		input_sync(u_dev);
	}

	pre_val = usb_buf[0];

	/* 重新提交urb */
	usb_submit_urb (usb_urb, GFP_KERNEL);
	
}

static int my_usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	int pipe;

	interface = intf->cur_altsetting;
	endpoint = &interface->endpoint[0].desc;//把除了端点0的第一个端点取出来


	/* 分配一个device结构体 */
	u_dev = input_allocate_device();
	/* 设置 */
	 /* 产生哪类事件 */
	 set_bit(EV_KEY,u_dev->evbit);
	 set_bit(EV_REP,u_dev->evbit);

	 /* 产生哪些事件 */
	 set_bit(KEY_L,u_dev->keybit);
	 set_bit(KEY_S,u_dev->keybit);
	 set_bit(KEY_ENTER,u_dev->keybit);
	 
	/* 注册 */
	input_register_device(u_dev);
	/* 硬件相关的代码 */
	/* 先要找到USB的源 */
	/* 数据传输三要素 :源 目的 长度 */
	/* 源 */
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);//这里是个宏这里面含有端点的类型以及端点的方向，pipe是个整型就是找到USB的端点地址以及USB分配的地址
	/* 长度 */
	len = endpoint->wMaxPacketSize;
	/* 目的 */
	usb_buf = usb_buffer_alloc(dev, len, GFP_ATOMIC, &usb_adr_phy);

	/* 使用这三个要素 */
	/* 这里需要用到urb USB请求块 */
	/* USB request block */
	/* urb用于组织每次数据传输的请求 */
	/* 分配urb */
	usb_urb = usb_alloc_urb(0, GFP_KERNEL);
	/* 设置 */
	usb_fill_int_urb(usb_urb, dev, pipe, usb_buf,
			 len,
			 my_usb_mouse_irq, NULL, endpoint->bInterval);
	usb_urb->transfer_dma = usb_adr_phy;
	usb_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	/* 使用提交 */
	usb_submit_urb (usb_urb, GFP_KERNEL);
	
	return 0;
}

static void my_usb_mouse_disconnect(struct usb_interface *intf)
{
	struct usb_device *dev = interface_to_usbdev(intf);

	usb_kill_urb(usb_urb);
	usb_free_urb(usb_urb);
	usb_buffer_free(dev,len,usb_buf,&usb_adr_phy);
	input_unregister_device(u_dev);
	input_free_device(u_dev);
}

static struct usb_driver my_usb_mouse_driver = {
	.name		= "my_usb_mouse",
	.probe		= my_usb_mouse_probe,
	.disconnect	= my_usb_mouse_disconnect,
	.id_table	= my_usb_mouse_id_table,
};

static int my_usb_mouse_init(void)
{
	usb_register(&my_usb_mouse_driver);
	return 0;
}
static void my_usb_mouse_exit(void)
{
	usb_deregister(&my_usb_mouse_driver);
}

module_init(my_usb_mouse_init);
module_exit(my_usb_mouse_exit);
MODULE_LICENSE("GPL");

