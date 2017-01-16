/*********************************************************************
* Copyright - 2005 SerComm Corporation. 

* All Rights Reserved. SerComm Corporation reserves the right to make 
* changes to this document without notice. SerComm Corporation makes 
* no warranty, representation or guarantee regarding the suitability 
* of its products for any particular purpose. SerComm Corporation 
* assumes no liability arising out of the application or use of any 
* product or circuit. SerComm Corporation specifically disclaims any 
* and all liability, including without limitation consequential or 
* incidental damages; neither does it convey any license under its 
* patent rights, nor the rights of others.
**********************************************************************/
#define EXPORT_SYMTAB

#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
#define MODEVERSIONS
#endif

#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>

#ifdef DEBUG
#define PRINTK(format,argument...) printk(format,##argument)
#else
#define PRINTK(format,argument...)
#endif
extern int (*fast_br_handle_frame_hook)(struct sk_buff *skb);
extern void (*fast_br_register)(struct net_device *);
extern void (*fast_br_unregister)(struct net_device *);
static char *if1 = NULL;
static char *if2 = NULL;
static char *br_if = NULL;
static struct net_device *if1_dev=NULL;
static struct net_device *if2_dev=NULL;
static struct net_device *br_if_dev=NULL;
extern rwlock_t dev_base_lock;
module_param(if1, charp, 0400);
module_param(if2, charp, 0400);
module_param(br_if, charp, 0400);
//MODULE_PARM( if1, "s" );
//MODULE_PARM( if2, "s" );

MODULE_LICENSE("Copyright");

static spinlock_t       device_lock;

static int pfast_br_handle_frame_hook(struct sk_buff *skb)
{
	if(skb->dev->br_port == NULL)
		return -1;

	spin_lock(&device_lock);

	if(if1_dev && if2_dev && ((skb->dev == if1_dev) || (skb->dev == if2_dev))){
		//Broadcast or Mulitcast   
		if((skb->mac.raw[0] & 1)){	
			spin_unlock(&device_lock);
			return -1;
		}
		if((skb->mac.raw[1]!=skb->dev->dev_addr[1]
			|| skb->mac.raw[2]!=skb->dev->dev_addr[2]
			|| skb->mac.raw[3]!=skb->dev->dev_addr[3]
			|| skb->mac.raw[4]!=skb->dev->dev_addr[4]
			|| skb->mac.raw[5]!=skb->dev->dev_addr[5])
			&& 
		   (skb->mac.raw[1]!=br_if_dev->dev_addr[1]
		    || skb->mac.raw[2]!=br_if_dev->dev_addr[2]
		    || skb->mac.raw[3]!=br_if_dev->dev_addr[3]
		    || skb->mac.raw[4]!=br_if_dev->dev_addr[4]
		    || skb->mac.raw[5]!=br_if_dev->dev_addr[5])
		  ){
				
//			printk("dev_name=%s\n",skb->dev->name);
//			printk("fast_br_mac:[%x]:[%x]:[%x]:[%x]:[%x]\n",skb->dev->dev_addr[1],skb->dev->dev_addr[2],skb->dev->dev_addr[3],skb->dev->dev_addr[4],skb->dev->dev_addr[5]);

			if(skb->dev == if1_dev)
				skb->dev = if2_dev;
			else
				skb->dev = if1_dev;

			skb_push(skb, ETH_HLEN);
			if (skb->dev->flags & IFF_UP)
				if1_dev = if2_dev; 
			spin_unlock(&device_lock);
			return 0;
		}
	}
	spin_unlock(&device_lock);
	return -1;
}

static void pfast_br_register(struct net_device *dev)
{
	spin_lock(&device_lock);
	if(strcmp(dev->name,if1)==0){
		if1_dev=NULL;
		printk("fast bridge:REG:%s 0x%x\n",if1,if1_dev);
	}else if(strcmp(dev->name,if2)==0){
		if2_dev=NULL;
		printk("fast bridge:REG:%s 0x%x\n",if2,if2_dev);
	}
	spin_unlock(&device_lock);
}
static void pfast_br_unregister(struct net_device *dev)
{
	spin_lock(&device_lock);
	if(strcmp(dev->name,if1)==0){
		if1_dev=dev;
		printk("fast bridge:UNREG:%s\n",if1);
	}else if(strcmp(dev->name,if2)==0){
		if2_dev=dev;
		printk("fast bridge:UNREG:%s\n",if2);
	}
	spin_unlock(&device_lock);
}

static int __init fast_br_init_module(void)
{
	if(!if1 || !if2)
		return -1;

	spin_lock(&device_lock);
	fast_br_register = pfast_br_register;	
	fast_br_unregister = pfast_br_unregister;	
	fast_br_handle_frame_hook = pfast_br_handle_frame_hook;
	read_lock(&dev_base_lock);
	if1_dev=__dev_get_by_name(if1);
	if2_dev=__dev_get_by_name(if2);
	br_if_dev=__dev_get_by_name(br_if);
	read_unlock(&dev_base_lock);
	spin_unlock(&device_lock);
	printk("fast bridge %s=0x%X %s=0x%X\n",if1,if1_dev,if2,if2_dev);
	return 0;
}
static void __exit fast_br_cleanup_module(void)
{
	spin_lock(&device_lock);
	fast_br_register = NULL;
	fast_br_unregister = NULL;	
	fast_br_handle_frame_hook = NULL;
	if1_dev=NULL;
	if2_dev=NULL;
	br_if_dev=NULL;
	spin_unlock(&device_lock);
	printk("fast bridge cleanup...\n");
}
module_init(fast_br_init_module);
module_exit(fast_br_cleanup_module);
	
