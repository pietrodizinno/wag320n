/* $Id: minissdp.h,v 1.1.1.1 2009-01-05 09:01:14 fred_fu Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */
#ifndef __MINISSDP_H__
#define __MINISSDP_H__

extern struct  service_type_uuid *known_service_types;
extern char uuidvalue[];
extern char root_device_path[];

#define ROOT_DEVICE 0
#define EMBED_DEVICE 1
#define SERVICE 2

struct service_type_uuid{
	char *service_name;
	char *uuid;
	int service_type;
};

int
OpenAndConfSSDPReceiveSocket(const char * ifaddr,
                             int n_add_listen_addr,
							 const char * * add_listen_addr);
int
OpenAndConfSSDPNotifySocket(const char * addr);

void
SendSSDPNotifies(int s, const char * host, unsigned short port);

void
ProcessSSDPRequest(int s, const char * host, unsigned short port);

int
SendSSDPGoodbye(int s);

#endif

