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


#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/times.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/jhash.h>
#include <asm/atomic.h>
#include <linux/ip.h>
#include "br_private.h"
#include "br_igmp.h"



#if defined(CONFIG_MIPS_BRCM)

int snooping = 0;

void addr_conv(unsigned char *in, char * out)
{
    sprintf(out, "%02x%02x%02x%02x%02x%02x", in[0], in[1], in[2], in[3], in[4], in[5]);
}


/* Define ipv6 multicast mac address to let them pass through control filtering.
 * All ipv6 multicast mac addresses start with 0x33 0x33. So control_filter
 * only need to compare the first 2 bytes of the address.
 */
mac_addr ipv6_mc_addr = {{0x33, 0x33, 0x00, 0x00, 0x00, 0x00}}; /* only the left two bytes are significant */

mac_addr upnp_addr = {{0x01, 0x00, 0x5e, 0x7f, 0xff, 0xfa}};
mac_addr sys1_addr = {{0x01, 0x00, 0x5e, 0x00, 0x00, 0x01}};
mac_addr sys2_addr = {{0x01, 0x00, 0x5e, 0x00, 0x00, 0x02}};
mac_addr ospf1_addr = {{0x01, 0x00, 0x5e, 0x00, 0x00, 0x05}};
mac_addr ospf2_addr = {{0x01, 0x00, 0x5e, 0x00, 0x00, 0x06}};
mac_addr ripv2_addr = {{0x01, 0x00, 0x5e, 0x00, 0x00, 0x09}};
mac_addr sys_addr = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

static int control_filter(unsigned char *dest)
{
   if ((!memcmp(dest, &upnp_addr, ETH_ALEN)) ||
       (!memcmp(dest, &sys1_addr, ETH_ALEN)) ||
       (!memcmp(dest, &sys2_addr, ETH_ALEN)) ||
       (!memcmp(dest, &ospf1_addr, ETH_ALEN)) ||
       (!memcmp(dest, &ospf2_addr, ETH_ALEN)) ||
       (!memcmp(dest, &sys_addr, ETH_ALEN)) ||
       (!memcmp(dest, &ripv2_addr, ETH_ALEN)) ||
       (!memcmp(dest, &ipv6_mc_addr, 2)))
      return 0;
   else
      return 1;
}

void brcm_conv_ip_to_mac(char *ipa, char *maca)
{
   maca[0] = 0x01;
   maca[1] = 0x00;
   maca[2] = 0x5e;
   maca[3] = 0x7F & ipa[1];
   maca[4] = ipa[2];
   maca[5] = ipa[3];

   return;
}

void br_process_igmpv3(struct net_bridge *br, struct sk_buff *skb, unsigned char *dest, struct igmpv3_report *report)
{
  struct igmpv3_grec *grec;
  int i;
  struct in_addr src;
  union ip_array igmpv3_mcast;
  int num_src;
  unsigned char tmp[6];
  struct net_bridge_mc_fdb_entry *mc_fdb;

  if(report) {
    grec = &report->grec[0];
    for(i = 0; i < report->ngrec; i++) {
      igmpv3_mcast.ip_addr = grec->grec_mca;
      brcm_conv_ip_to_mac(igmpv3_mcast.ip_ar, tmp);
      switch(grec->grec_type) {
        case IGMPV3_MODE_IS_INCLUDE:
        case IGMPV3_CHANGE_TO_INCLUDE:
        case IGMPV3_ALLOW_NEW_SOURCES:
          for(num_src = 0; num_src < grec->grec_nsrcs; num_src++) {
            src.s_addr = grec->grec_src[num_src];
            mc_fdb = br_mc_fdb_find(br, skb->dev->br_port, tmp, eth_hdr(skb)->h_source, &src);
            if((NULL != mc_fdb) && 
               (mc_fdb->src_entry.filt_mode == MCAST_EXCLUDE)) {
              br_mc_fdb_remove(br, skb->dev->br_port, tmp, eth_hdr(skb)->h_source, SNOOP_EX_CLEAR, &src);
            }
            else {
              br_mc_fdb_add(br, skb->dev->br_port, tmp, eth_hdr(skb)->h_source, SNOOP_IN_ADD, &src);
            }
          }

          if(0 == grec->grec_nsrcs) {
            src.s_addr = 0;
            br_mc_fdb_remove(br, skb->dev->br_port, tmp, eth_hdr(skb)->h_source, SNOOP_EX_CLEAR, &src);
          }
         break;
       
         case IGMPV3_MODE_IS_EXCLUDE:
         case IGMPV3_CHANGE_TO_EXCLUDE:
         case IGMPV3_BLOCK_OLD_SOURCES:
          for(num_src = 0; num_src < grec->grec_nsrcs; num_src++) {
            src.s_addr = grec->grec_src[num_src];
            mc_fdb = br_mc_fdb_find(br, skb->dev->br_port, tmp, eth_hdr(skb)->h_source, &src);
            if((NULL != mc_fdb) && 
               (mc_fdb->src_entry.filt_mode == MCAST_INCLUDE)) {
              br_mc_fdb_remove(br, skb->dev->br_port, tmp, eth_hdr(skb)->h_source, SNOOP_IN_CLEAR, &src);
            }
            else {
              br_mc_fdb_add(br, skb->dev->br_port, tmp, eth_hdr(skb)->h_source, SNOOP_EX_ADD, &src);
            }
          }

          if(0 == grec->grec_nsrcs) {
            src.s_addr = 0;
            br_mc_fdb_add(br, skb->dev->br_port, tmp, eth_hdr(skb)->h_source, SNOOP_EX_ADD, &src);
          }
        break;
      }
      grec = (struct igmpv3_grec *)((char *)grec + IGMPV3_GRP_REC_SIZE(grec));
    }
  }
  return;
}

int mc_forward(struct net_bridge *br, struct sk_buff *skb, unsigned char *dest,int forward, int clone)
{
	struct net_bridge_mc_fdb_entry *dst;
	struct list_head *lh;
	int status = 0;
	struct sk_buff *skb2;
	struct net_bridge_port *p;
	unsigned char tmp[6];
	struct igmpv3_report *report;
	struct iphdr *pip = skb->nh.iph;
	struct in_addr src;
        unsigned char igmp_type = 0;

	if (!snooping)
		return 0;

	if ((snooping == SNOOPING_BLOCKING_MODE) && control_filter(dest))
	    status = 1;

	if (skb->data[9] == IPPROTO_IGMP) {
	    // For proxy; need to add some intelligence here 
	    if (!br->proxy) {
		if(pip->ihl == 5) {
                  igmp_type = skb->data[20];
		} else {
                  igmp_type = skb->data[24];
		}
		if ((igmp_type == IGMPV2_HOST_MEMBERSHIP_REPORT) &&
		    (skb->protocol == __constant_htons(ETH_P_IP))) {
	            src.s_addr = 0;
		    br_mc_fdb_add(br, skb->dev->br_port, dest, eth_hdr(skb)->h_source, SNOOP_EX_ADD, &src);
                }
                else if((igmp_type == IGMPV3_HOST_MEMBERSHIP_REPORT) &&
                        (skb->protocol == __constant_htons(ETH_P_IP))) {
                    if(pip->ihl == 5) {
                      report = (struct igmpv3_report *)&skb->data[20];
                    }
                    else {
                      report = (struct igmpv3_report *)&skb->data[24];
                    }
                    if(report) {
                      br_process_igmpv3(br, skb, dest, report);
                    }
                }
		else if (igmp_type == IGMP_HOST_LEAVE_MESSAGE) {
		    tmp[0] = 0x01;
		    tmp[1] = 0x00;
		    tmp[2] = 0x5e;
                    if(pip->ihl == 5) {
                      tmp[3] = 0x7F & skb->data[24];
                      tmp[4] = skb->data[25];
                      tmp[5] = skb->data[26];
                    } 
                    else {
                      tmp[3] = 0x7F & skb->data[29];
                      tmp[4] = skb->data[30];
                      tmp[5] = skb->data[31];
                    }
	            src.s_addr = 0;
		    br_mc_fdb_remove(br, skb->dev->br_port, tmp, eth_hdr(skb)->h_source, SNOOP_EX_CLEAR, &src);
		}
		else
		    ;
	    }
	    return 0;
	}

	/*
	if (clone) {
		struct sk_buff *skb3;

		if ((skb3 = skb_clone(skb, GFP_ATOMIC)) == NULL) {
			br->statistics.tx_dropped++;
			return;
		}

		skb = skb3;
	}
	*/
	
	list_for_each_rcu(lh, &br->mc_list) {
	    dst = (struct net_bridge_mc_fdb_entry *) list_entry(lh, struct net_bridge_mc_fdb_entry, list);
	    if (!memcmp(&dst->addr, dest, ETH_ALEN)) {
              if((dst->src_entry.filt_mode == MCAST_INCLUDE) && 
                 (pip->saddr == dst->src_entry.src.s_addr)) {

		if (!dst->dst->dirty) {
		    skb2 = skb_clone(skb, GFP_ATOMIC);
		    if (forward)
			br_forward(dst->dst, skb2);
		    else
			br_deliver(dst->dst, skb2);
		}
		dst->dst->dirty = 1;
		status = 1;
              }
              else if(dst->src_entry.filt_mode == MCAST_EXCLUDE) {
                if((0 == dst->src_entry.src.s_addr) ||
                   (pip->saddr != dst->src_entry.src.s_addr)) {

		  if (!dst->dst->dirty) {
		    skb2 = skb_clone(skb, GFP_ATOMIC);
		    if (forward)
			br_forward(dst->dst, skb2);
		    else
			br_deliver(dst->dst, skb2);
		  }
		  dst->dst->dirty = 1;
		  status = 1;
                }
                else if(pip->saddr == dst->src_entry.src.s_addr) {
                  status = 1;
                }
              }
	    }
	}
	if (status) {
	    list_for_each_entry_rcu(p, &br->port_list, list) {
		p->dirty = 0;
	  }
	}

	if ((!forward) && (status))
	kfree_skb(skb);

	return status;
}

static void query_timeout(unsigned long ptr)
{
	struct net_bridge_mc_fdb_entry *dst;
	struct list_head *tmp;
	struct list_head *lh;
	struct net_bridge *br;
    
	br = (struct net_bridge *) ptr;

	spin_lock_bh(&br->mcl_lock);
	list_for_each_safe_rcu(lh, tmp, &br->mc_list) {
	    dst = (struct net_bridge_mc_fdb_entry *) list_entry(lh, struct net_bridge_mc_fdb_entry, list);
	    if (time_after_eq(jiffies, dst->tstamp)) {
		list_del_rcu(&dst->list);
		kfree(dst);
	    }
	}
	spin_unlock_bh(&br->mcl_lock);
		
	mod_timer(&br->igmp_timer, jiffies + TIMER_CHECK_TIMEOUT*HZ);		
}

static int br_mc_fdb_update(struct net_bridge *br, struct net_bridge_port *prt, unsigned char *dest, unsigned char *host, int mode, struct in_addr *src)
{
	struct net_bridge_mc_fdb_entry *dst;
	struct list_head *lh;
	int ret = 0;
	int filt_mode;

        if(mode == SNOOP_IN_ADD)
          filt_mode = MCAST_INCLUDE;
        else
          filt_mode = MCAST_EXCLUDE;
    
	list_for_each_rcu(lh, &br->mc_list) {
	    dst = (struct net_bridge_mc_fdb_entry *) list_entry(lh, struct net_bridge_mc_fdb_entry, list);
	    if ((!memcmp(&dst->addr, dest, ETH_ALEN)) &&
                (src->s_addr == dst->src_entry.src.s_addr) &&
                (filt_mode == dst->src_entry.filt_mode) && 
		(!memcmp(&dst->host, host, ETH_ALEN)) && 
                (dst->dst == prt)) {
		dst->tstamp = jiffies + QUERY_TIMEOUT*HZ;
		    ret = 1;
             }
	}
	
	return ret;
}

static struct net_bridge_mc_fdb_entry *br_mc_fdb_get(struct net_bridge *br, struct net_bridge_port *prt, unsigned char *dest, unsigned char *host, int mode, struct in_addr *src)
{
	struct net_bridge_mc_fdb_entry *dst;
	struct list_head *lh;
	int filt_mode;
    
        if(mode == SNOOP_IN_CLEAR)
          filt_mode = MCAST_INCLUDE;
        else
          filt_mode = MCAST_EXCLUDE;
          
	list_for_each_rcu(lh, &br->mc_list) {
	    dst = (struct net_bridge_mc_fdb_entry *) list_entry(lh, struct net_bridge_mc_fdb_entry, list);
	    if ((!memcmp(&dst->addr, dest, ETH_ALEN)) && 
                (!memcmp(&dst->host, host, ETH_ALEN)) &&
                (filt_mode == dst->src_entry.filt_mode) && 
                (dst->src_entry.src.s_addr == src->s_addr)) {
		if (dst->dst == prt)
		    return dst;
	    }
	}
	
	return NULL;
}

extern mac_addr upnp_addr;

int br_mc_fdb_add(struct net_bridge *br, struct net_bridge_port *prt, unsigned char *dest, unsigned char *host, int mode, struct in_addr *src)
{
	struct net_bridge_mc_fdb_entry *mc_fdb;

	//printk("--- add mc entry ---\n");

        if(!br || !prt || !dest || !host)
            return 0;

        if((SNOOP_IN_ADD != mode) && (SNOOP_EX_ADD != mode))             
            return 0;

	if (!memcmp(dest, &upnp_addr, ETH_ALEN))
	    return 0;
	    
	if (br_mc_fdb_update(br, prt, dest, host, mode, src))
	    return 0;
	    
	mc_fdb = kmalloc(sizeof(struct net_bridge_mc_fdb_entry), GFP_KERNEL);
	if (!mc_fdb)
	    return ENOMEM;
	memcpy(mc_fdb->addr.addr, dest, ETH_ALEN);
	memcpy(mc_fdb->host.addr, host, ETH_ALEN);
	memcpy(&mc_fdb->src_entry, src, sizeof(struct in_addr));
	mc_fdb->src_entry.filt_mode = 
                  (mode == SNOOP_IN_ADD) ? MCAST_INCLUDE : MCAST_EXCLUDE;
	mc_fdb->dst = prt;
	mc_fdb->tstamp = jiffies + QUERY_TIMEOUT*HZ;

	spin_lock_bh(&br->mcl_lock);
	list_add_tail_rcu(&mc_fdb->list, &br->mc_list);
	spin_unlock_bh(&br->mcl_lock);

	if (!br->start_timer) {
    	    init_timer(&br->igmp_timer);
	    br->igmp_timer.expires = jiffies + TIMER_CHECK_TIMEOUT*HZ;
	    br->igmp_timer.function = query_timeout;
	    br->igmp_timer.data = (unsigned long) br;
	    add_timer(&br->igmp_timer);
	    br->start_timer = 1;
	}

	return 1;
}

void br_mc_fdb_cleanup(struct net_bridge *br)
{
	struct net_bridge_mc_fdb_entry *dst;
	struct list_head *lh;
	struct list_head *tmp;
    
	spin_lock_bh(&br->mcl_lock);
	list_for_each_safe_rcu(lh, tmp, &br->mc_list) {
	    dst = (struct net_bridge_mc_fdb_entry *) list_entry(lh, struct net_bridge_mc_fdb_entry, list);
	    list_del_rcu(&dst->list);
	    kfree(dst);
	}
	spin_unlock_bh(&br->mcl_lock);
}

void br_mc_fdb_remove_grp(struct net_bridge *br, struct net_bridge_port *prt, unsigned char *dest)
{
	struct net_bridge_mc_fdb_entry *dst;
	struct list_head *lh;
	struct list_head *tmp;

	spin_lock_bh(&br->mcl_lock);
	list_for_each_safe_rcu(lh, tmp, &br->mc_list) {
	    dst = (struct net_bridge_mc_fdb_entry *) list_entry(lh, struct net_bridge_mc_fdb_entry, list);
	    if ((!memcmp(&dst->addr, dest, ETH_ALEN)) && (dst->dst == prt)) {
		list_del_rcu(&dst->list);
		kfree(dst);
	    }
	}
	spin_unlock_bh(&br->mcl_lock);
}

int br_mc_fdb_remove(struct net_bridge *br, struct net_bridge_port *prt, unsigned char *dest, unsigned char *host, int mode, struct in_addr *src)
{
	struct net_bridge_mc_fdb_entry *mc_fdb;

	//printk("--- remove mc entry ---\n");

        if((SNOOP_IN_CLEAR != mode) && (SNOOP_EX_CLEAR != mode))             
            return 0;
	
	if ((mc_fdb = br_mc_fdb_get(br, prt, dest, host, mode, src))) {
	    spin_lock_bh(&br->mcl_lock);
	    list_del_rcu(&mc_fdb->list);
	    kfree(mc_fdb);
	    spin_unlock_bh(&br->mcl_lock);

	    return 1;
	}
	
	return 0;
}

struct net_bridge_mc_fdb_entry *br_mc_fdb_find(struct net_bridge *br, struct net_bridge_port *prt, unsigned char *dest, unsigned char *host, struct in_addr *src)
{
	struct net_bridge_mc_fdb_entry *dst;
	struct list_head *lh;
    
	list_for_each_rcu(lh, &br->mc_list) {
	    dst = (struct net_bridge_mc_fdb_entry *) list_entry(lh, struct net_bridge_mc_fdb_entry, list);
	    if ((!memcmp(&dst->addr, dest, ETH_ALEN)) && 
                (!memcmp(&dst->host, host, ETH_ALEN)) &&
                (dst->src_entry.src.s_addr == src->s_addr)) {
		if (dst->dst == prt)
		    return dst;
	    }
	}
	
	return NULL;
}

void br_process_igmp_info(struct net_bridge *br, struct sk_buff *skb)
{
	char destS[16];
	char srcS[16];
	unsigned char tmp[6];
	unsigned char igmp_type = 0;
	char *extif = NULL;
	struct iphdr *pip = skb->nh.iph;
	unsigned char *src = eth_hdr(skb)->h_source;
	const unsigned char *dest = eth_hdr(skb)->h_dest;
	struct net_bridge_port *p = rcu_dereference(skb->dev->br_port);

	if (snooping && br->proxy) {
	  if (skb->data[9] == IPPROTO_IGMP) {
	    if(pip->ihl == 5) {
	      igmp_type = skb->data[20];
	    } else {
	      igmp_type = skb->data[24];
	    }

	    if (igmp_type == IGMP_HOST_LEAVE_MESSAGE) {
			
	      if(pip->ihl == 5) {
	        brcm_conv_ip_to_mac(&skb->data[24], tmp);
	      } else {
	        brcm_conv_ip_to_mac(&skb->data[28], tmp);
	      }

	      addr_conv(tmp, destS);
	    }
	    else {
	      addr_conv(dest, destS);
            }
	    addr_conv(src, srcS);

	    extif = kmalloc(BCM_IGMP_SNP_BUFSZ, GFP_ATOMIC);

	    if(extif) {
	      memset(extif, 0, BCM_IGMP_SNP_BUFSZ);
	      sprintf(extif, "%s %s %s/%s", 
                        br->dev->name, p->dev->name, destS, srcS);
	      skb->extif = extif;
	    }
	  }
	}

	return;
} /* br_process_igmp_info */
#endif
