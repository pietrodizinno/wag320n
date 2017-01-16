/* Kernel module to match a string into a packet.
 *
 * Copyright (C) 2000 Emmanuel Roger  <winfield@freegates.be>
 *
 * ChangeLog
 *	19.02.2002: Gianni Tedesco <gianni@ecsc.co.uk>
 *		Fixed SMP re-entrancy problem using per-cpu data areas
 *		for the skip/shift tables.
 *	02.05.2001: Gianni Tedesco <gianni@ecsc.co.uk>
 *		Fixed kernel panic, due to overrunning boyer moore string
 *		tables. Also slightly tweaked heuristic for deciding what
 * 		search algo to use.
 * 	27.01.2001: Gianni Tedesco <gianni@ecsc.co.uk>
 * 		Implemented Boyer Moore Sublinear search algorithm
 * 		alongside the existing linear search based on memcmp().
 * 		Also a quick check to decide which method to use on a per
 * 		packet basis.
 */


#include <linux/smp.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/file.h>
#include <net/sock.h>
#include <net/tcp.h>

#include <linux/netfilter/x_tables.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_httpReturnCode.h>

#define smp_num_cpus 1

struct returncode_per_cpu {
	int *skip;
	int *shift;
	int *len;
};

struct returncode_per_cpu *bm_string_data_get=NULL;

static int match(const struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		const struct xt_match *match,
		const void *matchinfo,
		int offset, unsigned int protoff, int *hotdrop)
{
	const struct ipt_httpReturnCode_info *info = matchinfo;
	const struct tcphdr *tcph = (void *)skb->nh.iph + skb->nh.iph->ihl*4;
	struct iphdr *ip = skb->nh.iph;
	int hlen, nlen;
	char *needle, *http, *httpEnd;
	int result = 0;
	char *p=NULL;

	if ( !ip ) return 0;

	/* get lenghts, and validate them */

	nlen = info->len;
	hlen = ntohs(ip->tot_len) - (ip->ihl*4);

	if ( nlen > hlen )
	{
		result =  0;
	}

	needle = (char *)&info->string;

	if( hlen - tcph->doff * 4 < 20 || hlen - tcph->doff * 4 < info->len )
	{
		result = 0;
	}
	else
	{
		http = (char *)tcph + tcph->doff * 4;
		httpEnd = (char *)ip + ntohs(ip->tot_len) - 1;
		p = http;
		if( strncmp( http, "HTTP/", sizeof("HTTP/") -1 ) != 0 )
		{
			result = 0;
		}
		else
		{
			p = http;
			p += sizeof("HTTP/") -1;

			while(*p != ' ' && p <= httpEnd && *p != '\r' && *p!= '\n')
				p ++;

			if(p > httpEnd || *p == '\r' || *p == '\n')
			{
				result = 0;
			}
			else
			{
			    while(*p == ' ' && p <= httpEnd && *p != '\r' && *p!= '\n')
				    p ++;

				if(p > httpEnd || *p == '\r' || *p == '\n')
				{
					result = 0;
				}
				else
				{
					if( strncmp( p, needle, info->len) == 0 )
					{
//						printk("<0> HTTP Find error Code = <%s>\n", needle);
						result = 1;
					}
					else
						result = 0;
				}
			}
		}
	}
	if(result == 1)
	{
//		printk("<0>######################################################Find 404 OK\n");
	}
	return (result ^ info->invert);
}

static int
checkentry(const char *tablename,
           const void *ip,
           const struct xt_match *match,
           void *matchinfo,
           unsigned int hook_mask)
{
//	if (matchsize != IPT_ALIGN(sizeof(struct ipt_httpReturnCode_info)))
//	{
//		printk("ipt_httpReturnCode: matchsize %u != %u\n", matchsize,
//		       IPT_ALIGN(sizeof(struct ipt_httpReturnCode_info)));
//		return 0;
//	}

	return 1;
}

static void string_freeup_data(void)
{
	int c;

	if ( bm_string_data_get ) {
		for(c=0; c<smp_num_cpus; c++) {
			if ( bm_string_data_get[c].shift ) kfree(bm_string_data_get[c].shift);
			if ( bm_string_data_get[c].skip ) kfree(bm_string_data_get[c].skip);
			if ( bm_string_data_get[c].len ) kfree(bm_string_data_get[c].len);
		}
		kfree(bm_string_data_get);
	}
}

static struct xt_match httpReturnCode_match =
{
    .name 		= "httpReturnCode",
    .family		= AF_INET,
    .match 		= &match,
    .matchsize	= sizeof(struct ipt_httpReturnCode_info),
    .checkentry = &checkentry,
    .destroy 	= NULL,
    .me 		= THIS_MODULE
};

static int __init init(void)
{
	int c;
	size_t tlen;
	size_t alen;

	tlen=sizeof(struct returncode_per_cpu)*smp_num_cpus;
	alen=sizeof(int)*BM_MAX_HLEN;

	/* allocate array of structures */
	if ( !(bm_string_data_get=kmalloc(tlen,GFP_KERNEL)) ) {
		return 0;
	}

	memset(bm_string_data_get, 0, tlen);

	/* allocate our skip/shift tables */
	for(c=0; c<smp_num_cpus; c++) {
		if ( !(bm_string_data_get[c].shift=kmalloc(alen, GFP_KERNEL)) )
			goto alloc_fail;
		if ( !(bm_string_data_get[c].skip=kmalloc(alen, GFP_KERNEL)) )
			goto alloc_fail;
		if ( !(bm_string_data_get[c].len=kmalloc(alen, GFP_KERNEL)) )
			goto alloc_fail;
	}

	if (xt_register_match(&httpReturnCode_match))
	{
//		printk("ipt_httpReturnCode match loaded unsuccessfully\n");
		return -EINVAL;	
	}
	
//	printk("ipt_httpReturnCode match loaded successfully\n");
	
	return 0;

alloc_fail:
//	printk("ipt_httpReturnCode match allocation failture\n");
	string_freeup_data();
	return 0;
}

static void __exit fini(void)
{
	xt_unregister_match(&httpReturnCode_match);
	string_freeup_data();
//	printk("ipt_httpReturnCode match unloaded\n");
}

module_init(init);
module_exit(fini);

#if 0

#include <linux/vermagic.h>
#include <linux/compiler.h>
MODULE_INFO(vermagic, VERMAGIC_STRING);

#undef unix
struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = __stringify(KBUILD_MODNAME),
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
};

static const char __module_depends[]
__attribute_used__
__attribute__((section(".modinfo"))) =
"depends=";
#endif
