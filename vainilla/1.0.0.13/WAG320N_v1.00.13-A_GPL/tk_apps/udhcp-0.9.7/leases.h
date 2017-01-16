/* leases.h */
#ifndef _LEASES_H
#define _LEASES_H
#include "packet.h"

#define LEASE_STATIC    (1<<0)
struct dhcpOfferedAddr {
	u_int8_t chaddr[16];
	u_int32_t yiaddr;	/* network order */
	u_int32_t expires;	/* host order */
#ifdef RONSCODE
	char hostname[256];
#endif
    int prop;
    u_int8_t if_flag;
}__attribute__ ((packed)) ;

enum
{
	ETHERNET_CLIENT,
	WLAN_CLIENT
};

extern unsigned char blank_chaddr[];

void clear_lease(u_int8_t *chaddr, u_int32_t yiaddr, int ifid, int serverid);
#ifdef RONSCODE
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, u_int32_t lease, int ifid, int serverid, \
		char *hostname, u_int8_t if_flag);
#else
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, u_int32_t lease, int ifid, int serverid);
#endif
int lease_expired(struct dhcpOfferedAddr *lease);
struct dhcpOfferedAddr *oldest_expired_lease(int ifid, int serverid);
struct dhcpOfferedAddr *find_lease_by_chaddr(u_int8_t *chaddr, int ifid, int serverid);
struct dhcpOfferedAddr *find_lease_by_yiaddr(u_int32_t yiaddr, int ifid, int serverid);
u_int32_t find_address(int check_expired, int ifid, int serverid);
int check_ip(u_int32_t addr, int ifid, int serverid);
int find_iface_by_chaddr(u_int8_t *chaddr,u_int8_t *if_flag);

#endif
