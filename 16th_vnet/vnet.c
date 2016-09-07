
#include <linux/module.h>

#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/ip.h>

#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>

static struct net_device *vnet_dev;

static int emulator_rx_packet(struct sk_buff *skb,struct net_device *dev)
{
	/* 参考LDD3 */
	unsigned char *type;
	struct iphdr *ih;
	__be32 *saddr, *daddr, tmp;
	unsigned char	tmp_dev_addr[ETH_ALEN];
	struct ethhdr *ethhdr;
	
	struct sk_buff *rx_skb;
	/* 源/目的 */
	ethhdr = (struct ethhdr *)skb->data;
	/* 因为是虚拟的，所以我们把源和 */
	memcpy(tmp_dev_addr,ethhdr->h_dest,ETH_ALEN);
	memcpy(ethhdr->h_dest,ethhdr->h_source,ETH_ALEN);
	memcpy(ethhdr->h_source,tmp_dev_addr,ETH_ALEN);
	/* 对调"源/目的"的ip地址 */    
	ih = (struct iphdr *)(skb->data + sizeof(struct ethhdr));
	saddr = &ih->saddr;
	daddr = &ih->daddr;

	tmp = *saddr;
	*saddr = *daddr;
	*daddr = tmp;

	type = skb->data + sizeof(struct ethhdr) + sizeof(struct iphdr);

	// 修改类型, 原来0x8表示ping
	*type = 0; /* 0表示reply */
	
	ih->check = 0;		   /* and rebuild the checksum (ip needs it) */
	ih->check = ip_fast_csum((unsigned char *)ih,ih->ihl);

	// 构造一个sk_buff
	rx_skb = dev_alloc_skb(skb->len + 2);
	skb_reserve(rx_skb, 2); /* align IP on 16B boundary */	
	memcpy(skb_put(rx_skb, skb->len), skb->data, skb->len);

	/* Write metadata, and then pass to the receive level */
	rx_skb->dev = dev;
	rx_skb->protocol = eth_type_trans(rx_skb, dev);
	rx_skb->ip_summed = CHECKSUM_UNNECESSARY; /* don't check it */
	dev->stats.rx_packets++;
	dev->stats.rx_bytes += skb->len;

	// 提交sk_buff
	netif_rx(rx_skb);
	
}

static int virt_net_send_packet (struct sk_buff *skb,struct net_device *dev)
{
	static int cnt = 0;
	printk("virt_net_send_packet cnt = %d\n",++cnt);
	

	/* 如果是真的网卡这里是用来把数skb中的数据通过网卡发送出去的 */
	netif_stop_queue(dev);
	/* 把skb的数据写入网卡 */

	/* 构造一个假的skb上报 */
	emulator_rx_packet(skb,dev);
	/* 传输结束 */
	dev_kfree_skb (skb);
	netif_wake_queue(dev);
	/* 更新系统信息 */
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	return 0;
}

static int virt_net_init(void)
{
	/* 分配结构体 */
	vnet_dev = alloc_netdev(0, "vnet%d", ether_setup);;
	/* 设置这个结构体 */
	vnet_dev->hard_start_xmit = virt_net_send_packet;

	/* 设置mac地址 */
	vnet_dev->dev_addr[0] = 0x08;
    vnet_dev->dev_addr[1] = 0x89;
    vnet_dev->dev_addr[2] = 0x89;
    vnet_dev->dev_addr[3] = 0x89;
    vnet_dev->dev_addr[4] = 0x89;
    vnet_dev->dev_addr[5] = 0x89;
	
	/* 注册 */
	register_netdev(vnet_dev);
	return 0;
}
static void virt_net_exit(void)
{
	unregister_netdev(vnet_dev);
	free_netdev(vnet_dev);
}
module_init(virt_net_init);
module_exit(virt_net_exit);
MODULE_LICENSE("GPL");


