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

#include <linux/netfilter_ipv4/ip_tables.h>
#include "ipt_httpMETHOD.h"
#define smp_num_cpus				1

struct httpmethod_per_cpu {
    int *skip;
    int *shift;
    int *len;
};
		
static int match(const struct sk_buff *skb,
		const struct net_device *in,
		const struct net_device *out,
		const struct xt_match *match,
		const void *matchinfo,
		int offset, unsigned int protoff, int *hotdrop)
{
    const struct ipt_httpmethod_info *info = matchinfo;
    const struct tcphdr *tcph = (void *)skb->nh.iph + skb->nh.iph->ihl*4;
    struct iphdr *ip = skb->nh.iph;
    int hlen, nlen;
    char *needle, *http;
    int result = 0;
    if ( !ip ) return 0;

    /* get lenghts, and validate them */
    nlen = info->len;
    hlen = ntohs(ip->tot_len) - (ip->ihl*4);

    if ( nlen > hlen ) 
    {
	return 0;
    }

    needle = (char *)&info->string;
 
    if( hlen - tcph->doff * 4 < 20 || hlen - tcph->doff * 4 < info->len )
    {
	result = 0;
    }
    else
    {
    	http = (char *)tcph + tcph->doff * 4;
	
	if( strncmp( http, needle, info->len ) == 0 )
	{	
	    result = 1;
	}
	else
	{
	    result = 0;
	}
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
    //if (matchsize != IPT_ALIGN(sizeof(struct ipt_httpmethod_info)))
	//	return 0;
    if (((struct ipt_ip *)ip)->proto != IPPROTO_TCP ) {
		printk("httpmethod: Only works on TCP packets\n");
		return 0;
    }
    return 1;
}


static struct xt_match httpmethod_match =
{ 
    .name = "httpmethod",
    .family		= AF_INET,
    .match = &match,
    .matchsize	= sizeof(struct ipt_httpmethod_info),
    .checkentry = &checkentry,
    .destroy = NULL,
    .me = THIS_MODULE
};
    
static int __init init(void)
{	
    return xt_register_match(&httpmethod_match);

    return 0;
}

static void __exit fini(void)
{
    xt_unregister_match(&httpmethod_match);
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
