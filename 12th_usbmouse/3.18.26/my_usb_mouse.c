#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

static struct input_dev *u_dev;
static int len;
static char *usb_buf;//usb�����������ݻ���
static dma_addr_t usb_buf_phy;
static struct urb *usb_urb;

static struct usb_device_id my_usb_mouse_id_table [] = {
	{ USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID, USB_INTERFACE_SUBCLASS_BOOT,
		USB_INTERFACE_PROTOCOL_MOUSE) },
	{ }	/* Terminating entry */
};

static void my_usb_mouse_irq(struct urb *urb)
{
	static char pre_val;
#if 0
	static int i;
	for(i = 0;i < len;i++)
	{
		printk("%x ",usb_buf[i]);
	}
	printk("\n");
#endif
	/*
	 *bit[0]:���
	 *bit[1]:�Ҽ�
	 *bit[2]:�м��
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
	/* �����ύ */
	usb_submit_urb(usb_urb, GFP_KERNEL);
}

static int my_usb_mouse_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	struct usb_host_interface *interface;
	struct usb_endpoint_descriptor *endpoint;
	int pipe;
	interface = intf->cur_altsetting;

	if (interface->desc.bNumEndpoints != 1)
		return -ENODEV;

	endpoint = &interface->endpoint[0].desc;
	if (!usb_endpoint_is_int_in(endpoint))
		return -ENODEV;

	/* ����input_dev�ṹ�� */
	u_dev = input_allocate_device();
	/* ���� */
	/* ���������¼� */
	set_bit(EV_KEY,u_dev->evbit);
	set_bit(EV_REP,u_dev->evbit);
	
	/* �����ĸ��¼� */
	set_bit(KEY_L,u_dev->keybit);
	set_bit(KEY_S,u_dev->keybit);
	set_bit(KEY_ENTER,u_dev->keybit);

	/* ע�� */
	input_register_device(u_dev);
	/* Ӳ����ش��� */
	/* ���ݴ�����Ҫ�� */
	/* Դ */
	pipe = usb_rcvintpipe(dev, endpoint->bEndpointAddress);
	/* ���� */
	len = endpoint->wMaxPacketSize;
	/* Ŀ�� */
	usb_buf = usb_alloc_coherent(dev, len, GFP_ATOMIC, &usb_buf_phy);
	/* ��urb����ռ� */
	usb_urb = usb_alloc_urb(0, GFP_KERNEL);
	/* ����urb */
	usb_fill_int_urb(usb_urb, dev, pipe, usb_buf,
			 len,
			 my_usb_mouse_irq, NULL, endpoint->bInterval);//bInterval��USB�豸�����Ƶ��
	usb_urb->transfer_dma = usb_buf_phy;
	usb_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	/* �ύurb */
	usb_submit_urb(usb_urb, GFP_KERNEL);
	return 0;
}

static void my_usb_mouse_disconnect(struct usb_interface *intf)
{
	struct usb_device *dev = interface_to_usbdev(intf);
	usb_kill_urb(usb_urb);
	usb_free_urb(usb_urb);
	usb_free_coherent(dev,len,usb_buf,&usb_buf_phy);
	input_unregister_device(u_dev);
	input_free_device(u_dev);
}

static struct usb_driver my_usb_mouse_driver = {
	.name		= "usbmouse",
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


