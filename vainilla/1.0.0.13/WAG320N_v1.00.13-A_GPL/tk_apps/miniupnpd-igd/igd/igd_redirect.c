/* $Id: igd_redirect.c,v 1.2 2009-09-30 00:44:32 fred_fu Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "igd_redirect.h"
#include "igd_globalvars.h"
#include "igd_permissions.h"
#if defined(__linux__)
#include "linux/iptcrdr.h"
#else
#include "openbsd/obsdrdr.h"
#endif

/* proto_atoi() */
static int
proto_atoi(const char * protocol)
{
	int proto = IPPROTO_TCP;
	if(strcmp(protocol, "UDP") == 0)
		proto = IPPROTO_UDP;
	return proto;
}

int
upnp_redirect(const char * eaddr,unsigned short eport, 
              const char * iaddr, unsigned short iport,
              const char * protocol, const char * desc)
{
	int proto, r;
	char iaddr_old[32];
	unsigned short iport_old;
	struct in_addr address;
	proto = proto_atoi(protocol);
	if(inet_aton(iaddr, &address) < 0)
	{
		syslog(LOG_ERR, "inet_aton(%s) : %m", iaddr);
		return -1;
	}
	if(!check_upnp_rule_against_permissions(upnppermlist, num_upnpperm,
	                                        eport, address, iport))
	{
		syslog(LOG_INFO, "redirection permission check failed for "
		                 "%hu->%s:%hu %s", eport, iaddr, iport, protocol);
		return -3;
	}
	r = get_redirect_rule(ext_if_name, eport, proto,
	                      iaddr_old, sizeof(iaddr_old), &iport_old, 0, 0, 0, 0);
	
	if(r == 0)
	{
		syslog(LOG_INFO, "port %hu protocol %s already redirected to %s:%hu",
			eport, protocol, iaddr_old, iport_old);
		
		if(strcmp(iaddr,iaddr_old)==0 && iport==iport_old)
		{
			syslog(LOG_INFO, "ignoring redirect request as it matches existing redirect");
		}
		else
		{
			syslog(LOG_INFO, "port %hu protocol %s already redirected to %s:%hu",
				eport, protocol, iaddr_old, iport_old);
			return -2;
		}
	}
	else
	{
		syslog(LOG_INFO, "redirecting port %hu to %s:%hu protocol %s for: %s",
			eport, iaddr, iport, protocol, desc);			
		if(add_redirect_rule2(ext_if_name,eaddr, eport, iaddr, iport, proto, desc) < 0)
		{
			return -1;
		}

		/* TODO: fix this stuff for both netfilter and pf */
		syslog(LOG_INFO, "creating pass rule to %s:%hu protocol %s for: %s",
			iaddr, iport, protocol, desc);
#if defined(__linux__)
		if(add_filter_rule2(ext_if_name, iaddr, iport, proto, desc) < 0)
#else
		if(add_filter_rule2(ext_if_name, iaddr, eport, proto, desc) < 0)
#endif
		{
			/* clean up the redirect rule */
#if !defined(__linux__)
			delete_redirect_rule(ext_if_name, eport, proto);
#endif
			return -1;
		}

	}

	return 0;
}

int
upnp_get_redirection_infos(unsigned short eport, const char * protocol,
                           unsigned short * iport,
                           char * iaddr, int iaddrlen,
                           char * desc, int desclen)
{
	if(desc && (desclen > 0))
		desc[0] = '\0';
	return get_redirect_rule(ext_if_name, eport, proto_atoi(protocol),
	                         iaddr, iaddrlen, iport, desc, desclen, 0, 0);
}

int
upnp_get_redirection_infos_by_index(int index,
                                    unsigned short * eport, char * protocol,
                                    unsigned short * iport, 
                                    char * iaddr, int iaddrlen,
                                    char * desc, int desclen)
{
	char ifname[IFNAMSIZ];
	int proto = 0;

	if(desc && (desclen > 0))
		desc[0] = '\0';
	if(get_redirect_rule_by_index(index, ifname, eport, iaddr, iaddrlen,
	                              iport, &proto, desc, desclen, 0, 0) < 0)
		return -1;
	else
	{
		if(proto == IPPROTO_TCP)
			memcpy(protocol, "TCP", 4);
		else
			memcpy(protocol, "UDP", 4);
		return 0;
	}
}

int
upnp_delete_redirection(unsigned short eport, const char * protocol)
{
	int proto, r;
	proto = proto_atoi(protocol);
	syslog(LOG_INFO, "removing redirect rule port %hu %s", eport, protocol);
#if defined(__linux__)
	r = delete_redirect_and_filter_rules(eport, proto);
#else
	r = delete_redirect_rule(ext_if_name, eport, proto);
	delete_filter_rule(ext_if_name, eport, proto);
#endif
	return r;
}

