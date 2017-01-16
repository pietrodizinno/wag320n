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

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/timer.h>
#include <linux/netlink.h>
#include <linux/proc_fs.h>
#include <linux/socket.h>
#include <net/sock.h>

#include "push_button.h"

struct timer_list pb_timer;
struct sock *nl_sk = NULL;
static int reset_button = 0, wps_button=0, reboot_button=0;
int pb_status=0;

void nl_data_ready (struct sock *sk, int len)
{
	printk("CallBack called \n");
}

unsigned int reset_count=0, wps_count=0;

void MACAddressAddNum(unsigned char *pMAC, int num, int len)
{
	unsigned long long val=0;
	int i=0;

	for(i=0; i<len; i++){
		val|=(unsigned long long)((unsigned long long)pMAC[i]<<((len-1-i)*8));
	}
	val+=(unsigned long long)num;
	for(i=0; i<len; i++)
		pMAC[i]=(val>>(len-1-i)*8) & 0xFF;
}

/* Just need add code to check button whether to be pushed */
/* If button is presswd, it should return 1 */
static int buttonIsPushed(void)
{
	if(!(GPIO_DATA_INPUT1 & GPIO_WPS_BUTTON)){
		GPIO_DATA_OUTPUT1 |= GPIO_WPS_BUTTON;
		wps_count++;
		if(wps_count>=3){
			wps_button=1;
		}
		return 1;
	}
	else
		wps_count=0;
	if(!(GPIO_DATA_INPUT & GPIO_RESET_BUTTON)){
		GPIO_DATA_OUTPUT |= GPIO_RESET_BUTTON;
		reset_count++;
		if(reset_count>=20){
			reset_button=1;
		}
		return 1;
	}	
	else{
		if(reset_count>=3 && reset_count<20)
			reboot_button=1;
		reset_count=0;
	}
    return 0;
}

static void init_gpio(void)
{
	GPIO_DATA_ENABLE |= GPIO_RESET_BUTTON;	
	GPIO_DATA_ENABLE1 |= GPIO_WPS_BUTTON;	
}
#if 0
void send_message(void)
{ 
	struct sk_buff *skb = NULL;
	int size;
	unsigned char *old_tail;
	struct nlmsghdr *nlh;
	struct pb_data data;

 	size = sizeof(data) + sizeof(struct nlmsghdr);
	skb=alloc_skb(NLMSG_SPACE(MAX_PAYLOAD),GFP_KERNEL);
	if(skb == NULL)
		return ;
	old_tail = skb->tail;
	nlh=NLMSG_PUT(skb,0,0,0,size-sizeof(*nlh));
	nlh = (struct nlmsghdr *)skb->data;
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nlh->nlmsg_pid = 0;  /* from kernel */
	nlh->nlmsg_flags = 0;
	data.now_staus = pb_status;
	memcpy((NLMSG_DATA(nlh)), &data,sizeof(data));
	nlh->nlmsg_len = skb->tail-old_tail;
	/* sender is in group 1<<0 */
	NETLINK_CB(skb).groups = 1;
	NETLINK_CB(skb).pid = 0;  /* from kernel */
	 NETLINK_CB(skb).dst_pid = 0;  /* multicast */
	/* to mcast group 1<<0 */
	NETLINK_CB(skb).dst_groups = 1;
	 /*multicast the message to all listening processes*/
	netlink_broadcast(nl_sk, skb, 0, 1, GFP_KERNEL);
	return ;
nlmsg_failure:			/* Used by NLMSG_PUT */
	if (skb)
		kfree_skb(skb);
	return;
}
#endif
/*********************/
void gpio_timer_callback(unsigned long data)
{
	buttonIsPushed();
#if 0	
	if(now_status != pb_status){   
		pb_status = now_status;
		printk("push button's status is changed\n");
		send_message();      
	}
#endif	
	//GPIO_DATA_ENABLE |= GPIO_RESET_BUTTON;	
	//GPIO_DATA_ENABLE1 |= GPIO_WPS_BUTTON;
	/* Restart the timer */
	mod_timer( &pb_timer , jiffies + 20 ) ;
}

int push_button_read_proc(struct file *filp,char *buf,size_t count , loff_t *offp)
{
	int len=0;
	
	if(*offp!=0)
		return 0;
	if(wps_button){
		len=sprintf(buf, "%s", "WPS");
		wps_button=0;
	}
	else if(reset_button){
		len=sprintf(buf, "%s", "RESET");
		reset_button=0;
	}
	else if(reboot_button){
		len=sprintf(buf, "%s", "REBOOT");
		reboot_button=0;
	}	
	else
		return 0;
	*offp=len;
	return len;
}

static ssize_t proc_read_macaddr_fops(struct file *filp, char *buf,size_t count , loff_t *offp)
{
	int len=0;
	unsigned char mac[MAX_ADDR_LEN];
	char macaddr[18]="";

	memset(mac,0,MAX_ADDR_LEN);
	if(*offp!=0)
		return 0;
	
	kerSysGetMacAddress(mac,7);
	//printk("mac: %02X:%02X:%02X:%02X:%02X:%02X\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	MACAddressAddNum(mac, 2, 6);
	len=sprintf(macaddr,"%02X:%02X:%02X:%02X:%02X:%02X\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);		
	macaddr[len]='\0';	
	copy_to_user(buf,macaddr,len);
	*offp = len;
	return len;
}

static ssize_t proc_read_wmacaddr_fops(struct file *filp, char *buf,size_t count , loff_t *offp)
{
	int len=0;
	unsigned char mac[MAX_ADDR_LEN];
	char macaddr[18]="";

	memset(mac,0,MAX_ADDR_LEN);
	if(*offp!=0)
		return 0;
	
	kerSysGetMacAddress(mac,7);
	MACAddressAddNum(mac, 1, 6);
	len=sprintf(macaddr,"%02X:%02X:%02X:%02X:%02X:%02X\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);		
	macaddr[len]='\0';	
	copy_to_user(buf,macaddr,len);
	*offp = len;
	return len;
}

static struct proc_dir_entry *push_button_proc = NULL;
static struct proc_dir_entry *proc_macaddr = NULL;
static struct proc_dir_entry *proc_wmacaddr = NULL;

static struct file_operations push_button_fops=
{
	read: push_button_read_proc,
};

struct file_operations get_macaddr_fops = {
        read: proc_read_macaddr_fops,
};

struct file_operations get_wmacaddr_fops = {
        read: proc_read_wmacaddr_fops,
};

int my_init(void)
{
	init_timer(&pb_timer);
	init_gpio();

	proc_macaddr=create_proc_entry("mac",0666,&proc_root);
	proc_macaddr->owner = THIS_MODULE;
	proc_macaddr->proc_fops = &get_macaddr_fops;
	
	proc_wmacaddr=create_proc_entry("wmac",0666,&proc_root);
	proc_wmacaddr->owner = THIS_MODULE;
	proc_wmacaddr->proc_fops = &get_wmacaddr_fops;
		
	push_button_proc=create_proc_entry("push_button",0666,&proc_root);
	push_button_proc->owner = THIS_MODULE;
	push_button_proc->proc_fops = &push_button_fops;
	
	pb_timer.expires=jiffies + 20;           /* In jiffies. One jiffie equal to 10ms */
	pb_timer.function= gpio_timer_callback;
	pb_timer.data=(unsigned long)&pb_timer;
#if 0	
	nl_sk = netlink_kernel_create(NETLINK_PB,nl_data_ready); 
	if(nl_sk == NULL)
		printk("open netlink socket fail\n");
#endif		
	add_timer(&pb_timer);
	
	return 0;
}

void my_exit(void)
{
 	/* Cancel Timer Here */
 	del_timer(&pb_timer);
	//sock_release(nl_sk->sk_socket);
	remove_proc_entry("push_button", &proc_root);
	remove_proc_entry("mac", &proc_root);
	remove_proc_entry("wmac", &proc_root);
}

module_init(my_init);
module_exit(my_exit);

