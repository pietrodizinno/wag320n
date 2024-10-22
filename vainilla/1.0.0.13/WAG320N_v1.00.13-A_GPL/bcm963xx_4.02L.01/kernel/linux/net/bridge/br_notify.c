/*
 *	Device event handling
 *	Linux ethernet bridge
 *
 *	Authors:
 *	Lennert Buytenhek		<buytenh@gnu.org>
 *
 *	$Id: br_notify.c,v 1.1.1.1 2009-01-05 09:00:42 fred_fu Exp $
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/rtnetlink.h>

#include "br_private.h"

static int br_device_event(struct notifier_block *unused, unsigned long event, void *ptr);

struct notifier_block br_device_notifier = {
	.notifier_call = br_device_event
};

/*
 * Handle changes in state of network devices enslaved to a bridge.
 *
 * Note: don't care about up/down if bridge itself is down, because
 *     port state is checked when bridge is brought up.
 */
static int br_device_event(struct notifier_block *unused, unsigned long event, void *ptr)
{
	struct net_device *dev = ptr;
	struct net_bridge_port *p = dev->br_port;
	struct net_bridge *br;

	/* not a port of a bridge */
	if (p == NULL)
		return NOTIFY_DONE;

	br = p->br;

	switch (event) {
	case NETDEV_CHANGEMTU:
		dev_set_mtu(br->dev, br_min_mtu(br));
		break;

	case NETDEV_CHANGEADDR:
		spin_lock_bh(&br->lock);
		br_fdb_changeaddr(p, dev->dev_addr);
		br_ifinfo_notify(RTM_NEWLINK, p);
		br_stp_recalculate_bridge_id(br);
		spin_unlock_bh(&br->lock);
		break;

	case NETDEV_CHANGE:
		br_port_carrier_check(p);
		break;

	case NETDEV_FEAT_CHANGE:
		spin_lock_bh(&br->lock);
		if (netif_running(br->dev))
			br_features_recompute(br);
		spin_unlock_bh(&br->lock);
		break;

	case NETDEV_DOWN:
		spin_lock_bh(&br->lock);
		if (br->dev->flags & IFF_UP)
			br_stp_disable_port(p);
		spin_unlock_bh(&br->lock);
		break;

	case NETDEV_UP:
		spin_lock_bh(&br->lock);
		if (netif_carrier_ok(dev) && (br->dev->flags & IFF_UP))
			br_stp_enable_port(p);
		spin_unlock_bh(&br->lock);
		break;

	case NETDEV_UNREGISTER:
		br_del_if(br, dev);
		break;
	}

	return NOTIFY_DONE;
}
