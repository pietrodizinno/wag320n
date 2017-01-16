/* $Id: upnphttp.h,v 1.1.1.1 2009-01-05 09:01:14 fred_fu Exp $ */ 
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef __UPNPHTTP_H__
#define __UPNPHTTP_H__

#include <sys/queue.h>

/*
 states :
  0 - waiting for data to read
  1 - waiting for HTTP Post Content.
  ...
  >= 100 - to be deleted
*/

enum httpCommands {
	EUnknown = 0,
	EGet,
	EPost
};

struct upnphttp {
	int socket;
	int state;
	char HttpVer[16];
	/* request */
	char * req_buf;
	int req_buflen;
	int req_contentlen;
	int req_contentoff;     /* header length */
	enum httpCommands req_command;
	char * req_soapAction;
	int req_soapActionLen;
	/* response */
	char * res_buf;
	int res_buflen;
	int res_buf_alloclen;
	/*int res_contentlen;*/
	/*int res_contentoff;*/		/* header length */
	LIST_ENTRY(upnphttp) entries;
};

struct time_list {
	char  protocol[20];
	unsigned short eport;
	time_t add_time;
	time_t timeout;
	LIST_ENTRY(time_list) entries;
};
LIST_HEAD(Time_list, time_list) time_head;

#endif

