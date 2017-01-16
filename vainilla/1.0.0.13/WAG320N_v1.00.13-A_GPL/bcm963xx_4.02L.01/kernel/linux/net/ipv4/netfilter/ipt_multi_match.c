/******************************************************************************************
 *
 *  Copyright - 2005 SerComm Corporation.
 *
 *	All Rights Reserved. 
 *	SerComm Corporation reserves the right to make changes to this document without notice.
 *	SerComm Corporation makes no warranty, representation or guarantee regarding 
 *	the suitability of its products for any particular purpose. SerComm Corporation assumes 
 *	no liability arising out of the application or use of any product or circuit.
 *	SerComm Corporation specifically disclaims any and all liability, 
 *	including without limitation consequential or incidental damages; 
 *	neither does it convey any license under its patent rights, nor the rights of others.
 *
 *****************************************************************************************/

#include <linux/smp.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/file.h>
#include <net/sock.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include "ipt_multi_match.h"
//#define MULTI_MATCH_DEBUG
static int
match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
      const struct xt_match *match,
      const void *matchinfo,
      int offset,
      unsigned int protoff,
      int *hotdrop)
{
	const struct ipt_multi_match_info *info = matchinfo;
	struct iphdr *ip = skb->nh.iph;
	unsigned char *mac;
	int find = 0;
	int i = -1;

	mac = eth_hdr(skb)->h_source;
#ifdef MULTI_MATCH_DEBUG
	printk("<0>""\nbegin to find multi match\n");
	printk("<0>""packet mac =%02x%02x%02x%02x%02x%02x\n",mac[0],
	mac[1],mac[2],mac[3],mac[4],mac[5]);
#endif
	if(mac != NULL)
	{
		for(i=0; i < MAX_MAC_NUM; i++)
			if(memcmp(mac,info->MAC[i],6) == 0)
			{
				find = 1;
#ifdef MULTI_MATCH_DEBUG
				printk("<0>""find mac match i=%d\n",i);
#endif
				goto last;
			}
	}

	if(ip == NULL)
		goto last;
	else
	{
#ifdef MULTI_MATCH_DEBUG
	printk("<0>""packet ip_addr =%x\n",ip->saddr);
#endif
		for(i=0; i < MAX_IP_NUM; i++)
			if(memcmp(&(ip->saddr),&(info->ip_addr[i]),4) == 0)
			{
				find = 1;
#ifdef MULTI_MATCH_DEBUG
				printk("<0>""find ip match i=%d,ip=%x\n",i,ip->saddr);
#endif
				goto last;
			}
#ifdef MULTI_MATCH_DEBUG
	printk("<0>""begin to find ip in ip range\n");
#endif
		for(i=0; i < MAX_IP_RANGE_NUM; i++)
			if((ntohl(ip->saddr) >= ntohl(info->ip_range[i][0])) && (ntohl(ip->saddr) <= ntohl(info->ip_range[i][1]) ))
			{
				find = 1;
#ifdef MULTI_MATCH_DEBUG
				printk("<0>""ip=%x.min_ip=%x,max_ip=%x\n",ntohl(ip->saddr),ntohl(info->ip_range[0]),ntohl(info->ip_range[1]));
				printk("<0>""find ip range match i=%d\n",i);
#endif
				goto last;
			}
	}
last:
	return (find ^ info->invert);
}

static int
checkentry(const char *tablename,
//           const void *ip,
           const struct ipt_ip *ip,
           void *matchinfo,
           unsigned int matchsize,
           unsigned int hook_mask)
{

       if (matchsize != IPT_ALIGN(sizeof(struct ipt_multi_match_info)))
               return 0;

       return 1;
}

static struct ipt_match multi_match =
{
    .name 	= "multi_match",
    .family	= AF_INET,
    .match 	= match,
    .matchsize	= sizeof(struct ipt_multi_match_info),
    .destroy = NULL,
    .me 	= THIS_MODULE
};

static int __init init(void)
{
	return xt_register_match(&multi_match);;
}

static void __exit fini(void)
{
	xt_unregister_match(&multi_match);
}

module_init(init);
module_exit(fini);
