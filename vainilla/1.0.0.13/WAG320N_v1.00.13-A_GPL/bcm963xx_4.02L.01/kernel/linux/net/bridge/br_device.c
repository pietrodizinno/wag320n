/*
 *	Device handling code
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	$Id: br_device.c,v 1.2 2009-07-15 11:26:07 shearer_lu Exp $
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>

#include <asm/uaccess.h>
#include "br_private.h"

static struct net_device_stats *br_dev_get_stats(struct net_device *dev)
{
	struct net_bridge *br = netdev_priv(dev);
	return &br->statistics;
}

/* net device transmit always called with no BH (preempt_disabled) */
int br_dev_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct net_bridge *br = netdev_priv(dev);
	const unsigned char *dest = skb->data;
	struct net_bridge_fdb_entry *dst;

	br->statistics.tx_packets++;
	br->statistics.tx_bytes += skb->len;

	skb->mac.raw = skb->data;
	skb_pull(skb, ETH_HLEN);

	if (dest[0] & 1) {
#if defined(CONFIG_MIPS_BRCM)
		if (!mc_forward(br, skb, (unsigned char *) dest, 0, 0))		
#endif
		br_flood_deliver(br, skb, 0);
	}
	else if ((dst = __br_fdb_get(br, dest)) != NULL)
		br_deliver(dst->dst, skb);
	else
		br_flood_deliver(br, skb, 0);

	return 0;
}

static int br_dev_open(struct net_device *dev)
{
	struct net_bridge *br = netdev_priv(dev);

	br_features_recompute(br);
	netif_start_queue(dev);
	br_stp_enable_bridge(br);

	return 0;
}

static void br_dev_set_multicast_list(struct net_device *dev)
{
}

static int br_dev_stop(struct net_device *dev)
{
	br_stp_disable_bridge(netdev_priv(dev));

	netif_stop_queue(dev);

	return 0;
}

static int br_change_mtu(struct net_device *dev, int new_mtu)
{
	if (new_mtu < 68 || new_mtu > br_min_mtu(netdev_priv(dev)))
		return -EINVAL;

	dev->mtu = new_mtu;
	return 0;
}

/* Allow setting mac address of pseudo-bridge to be same as
 * any of the bound interfaces
 */
static int br_set_mac_address(struct net_device *dev, void *p)
{
	struct net_bridge *br = netdev_priv(dev);
	struct sockaddr *addr = p;
	struct net_bridge_port *port;
	int err = -EADDRNOTAVAIL;

	spin_lock_bh(&br->lock);
	list_for_each_entry(port, &br->port_list, list) {
		if (!compare_ether_addr(port->dev->dev_addr, addr->sa_data)) {
			br_stp_change_bridge_id(br, addr->sa_data);
			err = 0;
			break;
		}
	}
	spin_unlock_bh(&br->lock);

	return err;
}

static void br_getinfo(struct net_device *dev, struct ethtool_drvinfo *info)
{
	strcpy(info->driver, "bridge");
	strcpy(info->version, BR_VERSION);
	strcpy(info->fw_version, "N/A");
	strcpy(info->bus_info, "N/A");
}

static int br_set_sg(struct net_device *dev, u32 data)
{
	struct net_bridge *br = netdev_priv(dev);

	if (data)
		br->feature_mask |= NETIF_F_SG;
	else
		br->feature_mask &= ~NETIF_F_SG;

	br_features_recompute(br);
	return 0;
}

static int br_set_tso(struct net_device *dev, u32 data)
{
	struct net_bridge *br = netdev_priv(dev);

	if (data)
		br->feature_mask |= NETIF_F_TSO;
	else
		br->feature_mask &= ~NETIF_F_TSO;

	br_features_recompute(br);
	return 0;
}

static int br_set_tx_csum(struct net_device *dev, u32 data)
{
	struct net_bridge *br = netdev_priv(dev);

	if (data)
		br->feature_mask |= NETIF_F_NO_CSUM;
	else
		br->feature_mask &= ~NETIF_F_ALL_CSUM;

	br_features_recompute(br);
	return 0;
}

static struct ethtool_ops br_ethtool_ops = {
	.get_drvinfo = br_getinfo,
	.get_link = ethtool_op_get_link,
	.get_sg = ethtool_op_get_sg,
	.set_sg = br_set_sg,
	.get_tx_csum = ethtool_op_get_tx_csum,
	.set_tx_csum = br_set_tx_csum,
	.get_tso = ethtool_op_get_tso,
	.set_tso = br_set_tso,
};

void br_dev_setup(struct net_device *dev)
{
	memset(dev->dev_addr, 0, ETH_ALEN);

	ether_setup(dev);

	dev->do_ioctl = br_dev_ioctl;
	dev->get_stats = br_dev_get_stats;
	dev->hard_start_xmit = br_dev_xmit;
	dev->open = br_dev_open;
	dev->set_multicast_list = br_dev_set_multicast_list;
	dev->change_mtu = br_change_mtu;
	dev->destructor = free_netdev;
	SET_MODULE_OWNER(dev);
	SET_ETHTOOL_OPS(dev, &br_ethtool_ops);
	dev->stop = br_dev_stop;
	dev->tx_queue_len = 0;
	dev->set_mac_address = br_set_mac_address;
	dev->priv_flags = IFF_EBRIDGE;

	dev->features = NETIF_F_SG | NETIF_F_FRAGLIST | NETIF_F_HIGHDMA |
			NETIF_F_TSO | NETIF_F_NO_CSUM | NETIF_F_GSO_ROBUST;
}
char *get_dev_port(struct sk_buff *skb)
{
    struct net_device *dev = skb->dev;
        struct net_bridge *br = netdev_priv(skb->dev);
        const unsigned char *dest = skb->data;
        struct net_bridge_fdb_entry *dst;

        if (dest[0] & 1)
                return dev->name;
        else if ((dst = __br_fdb_get(br, dest)) != NULL)
                return dst->dst->dev->name;
        else
                return dev->name;
}

EXPORT_SYMBOL(get_dev_port);

