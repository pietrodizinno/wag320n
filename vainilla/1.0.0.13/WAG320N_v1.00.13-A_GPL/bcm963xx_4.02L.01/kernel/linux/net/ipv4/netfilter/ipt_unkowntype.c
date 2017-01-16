/* 
This program is free software; you can distribute it and/or modify it
 under the terms of the GNU General Public License (Version 2) as
 published by the Free Software Foundation.

 This program is distributed in the hope it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
*/
#include <linux/smp.h>
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/file.h>
#include <net/sock.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_unkowntype.h>
//#include <linux/netfilter_ipv4/ip_conntrack_protocol.h>

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_l3proto.h>
#include <net/netfilter/nf_conntrack_l4proto.h>
#include <net/netfilter/nf_conntrack_core.h>

#define smp_num_cpus 1

struct string_per_cpu {
	int *skip;
	int *shift;
	int *len;
};

static struct string_per_cpu *bm_string_data_get=NULL;

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
	const struct ipt_unkowntype_info *info = matchinfo;
	struct iphdr *ip = skb->nh.iph;
	struct nf_conn *ct;
	enum ip_conntrack_info ctinfo;
	struct nf_conntrack_l4proto *l4proto;
	//struct ip_conntrack_protocol *proto;
	
	u_int16_t ret;
	if ( !ip ) return 0;

	ct = nf_ct_get(skb, &ctinfo);
	if (ct == NULL) {
		l4proto = nf_ct_l4proto_find_get(PF_INET, ip->protocol);
	} else {
		l4proto = nf_ct_l4proto_find_get(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.l3num,
					       ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.protonum);
		if (l4proto == NULL || l4proto->name == NULL) {
			printk("<0> UNKOWNTYPE %s is null\n", l4proto==NULL?"l4proto":"l4proto->name");
			return 0;
		}
	}

	//proto = __ip_conntrack_proto_find(ip->protocol);
	
	ret = ( (strcmp( l4proto->name, "unknown") == 0) == info->unkownprotocol );
	if (ret && ip->protocol == 0x32)	/* IPSEC packets, for cdrouter test */
		ret = 0;

	if(ret == 1)
	{
		printk("<0> UNKOWNTYPE BLOCK THIS PACKET: Protocol:%d[0x%x]\n",ip->protocol,ip->protocol);
	} 
    return ( ret ^ info->invert);
}

static int
checkentry(const char *tablename,
           const void *ip,
	   const struct xt_match *match,
           void *matchinfo,
           unsigned int hook_mask)
{

	/*
       if (matchsize != IPT_ALIGN(sizeof(struct ipt_unkowntype_info)))
               return 0;
	       */

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

static struct ipt_match unkowntype =
{ 
    .name = "unkowntype",
    .family = AF_INET,
    .match = match,
    .matchsize	= sizeof(struct ipt_unkowntype_info),
    .checkentry = checkentry,
    .destroy = NULL,
    .me = THIS_MODULE    
};

static int __init init(void)
{
	int c;
	size_t tlen;
	size_t alen;

	tlen=sizeof(struct string_per_cpu)*smp_num_cpus;
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
	
	return xt_register_match(&unkowntype);

alloc_fail:
	string_freeup_data();
	return 0;
}

static void __exit fini(void)
{
	xt_unregister_match(&unkowntype);
	string_freeup_data();
}

module_init(init);
module_exit(fini);
