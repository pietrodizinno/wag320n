/*****************************************************************************
<:copyright-gpl
 Copyright 2002 Broadcom Corp. All Rights Reserved.

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
:>
******************************************************************************
//
//  Filename:       ip_conntrack_ipsec.h
//  Author:         Pavan Kumar
//  Creation Date:  05/27/04
//
//  Description:
//      Implements the IPSec ALG connectiontracking data structures.
//
*****************************************************************************/
#ifndef _IP_CONNTRACK_IPSEC_H
#define _IP_CONNTRACK_IPSEC_H
/* FTP tracking. */

#ifndef __KERNEL__
#error Only in kernel.
#endif

#include <linux/netfilter_ipv4/lockhelp.h>

#define IPSEC_UDP_PORT 500

/* Protects ftp part of conntracks */
DECLARE_LOCK_EXTERN(ip_ipsec_lock);

struct isakmphdr {
	u_int32_t initcookie[2];
	u_int32_t respcookie[2];
};

/* This structure is per expected connection */
struct ip_ct_ipsec_expect
{
	/* We record initiator cookie and source IP address: all in
	 * host order. */

 	/* source cookie */
	u_int32_t initcookie[2];	/* initiator cookie */
	u_int32_t respcookie[2];	/* initiator cookie */
	u_int32_t saddr; 		/* source IP address in the orig dir */
};

/* This structure exists only once per master */
struct ip_ct_ipsec_master {
	u_int32_t initcookie[2];
	u_int32_t saddr;
};

#endif /* _IP_CONNTRACK_IPSEC_H */
