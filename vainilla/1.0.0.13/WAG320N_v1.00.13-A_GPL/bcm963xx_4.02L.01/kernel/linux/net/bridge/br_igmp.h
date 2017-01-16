/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef _BR_IGMP_H
#define _BR_IGMP_H

#include <linux/netdevice.h>
#include <linux/if_bridge.h>
#include <linux/igmp.h>
#include <linux/in.h>

#if defined(CONFIG_MIPS_BRCM)
#define SNOOPING_BLOCKING_MODE 2
#endif

union ip_array {
	unsigned int ip_addr;
        unsigned char ip_ar[4];
};


#if defined(CONFIG_MIPS_BRCM)
#define TIMER_CHECK_TIMEOUT 10
#define QUERY_TIMEOUT 130
//#define QUERY_TIMEOUT 60
#endif


#define IGMPV3_GRP_REC_SIZE(x)  (sizeof(struct igmpv3_grec) + \
                       (sizeof(struct in_addr) * ((struct igmpv3_grec *)x)->grec_nsrcs))

/* these definisions are also there igmprt.h */
#define SNOOP_IN_ADD		1
#define SNOOP_IN_CLEAR		2
#define SNOOP_EX_ADD		3
#define SNOOP_EX_CLEAR		4

struct net_bridge_mc_src_entry
{
	struct in_addr		src;
	unsigned long		tstamp;
        int			filt_mode;
};

struct net_bridge_mc_fdb_entry
{
	struct net_bridge_port		*dst;
	mac_addr			addr;
	mac_addr			host;
	struct net_bridge_mc_src_entry  src_entry;
	unsigned char			is_local;
	unsigned char			is_static;
	unsigned long			tstamp;
	struct list_head 		list;
};

extern int snooping;
extern int mc_forward(struct net_bridge *br, struct sk_buff *skb, unsigned char *dest,int forward, int clone);
extern int br_mc_fdb_add(struct net_bridge *br, struct net_bridge_port *prt, unsigned char *dest, unsigned char *host, int mode, struct in_addr *src);
extern void br_mc_fdb_cleanup(struct net_bridge *br);
extern void br_mc_fdb_remove_grp(struct net_bridge *br, struct net_bridge_port *prt, unsigned char *dest);
extern int br_mc_fdb_remove(struct net_bridge *br, struct net_bridge_port *prt, unsigned char *dest, unsigned char *host, int mode, struct in_addr *src);
extern struct net_bridge_mc_fdb_entry *br_mc_fdb_find(struct net_bridge *br, 
                                               struct net_bridge_port *prt, 
                                               unsigned char *dest, 
                                               unsigned char *host, 
                                               struct in_addr *src);
void br_process_igmp_info(struct net_bridge *br, struct sk_buff *skb);
void addr_conv(unsigned char *in, char * out);
void brcm_conv_ip_to_mac(char *ipa, char *maca);
#endif /* _BR_IGMP_H */
