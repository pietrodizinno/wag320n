/* $Id: upnphttp_func.h,v 1.1.1.1 2009-01-05 09:01:14 fred_fu Exp $ */ 
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef __UPNPHTTP_FUNC_H__
#define __UPNPHTTP_FUNC_H__

/* New_upnphttp() */
struct upnphttp *
New_upnphttp(int);

/* CloseSocket_upnphttp() */
void
CloseSocket_upnphttp(struct upnphttp *);

/* Delete_upnphttp() */
void
Delete_upnphttp(struct upnphttp *);

/* Process_upnphttp() */
void
Process_upnphttp(struct upnphttp *, struct eventlisthead *, struct event_list *list);

/* BuildHeader_upnphttp()
 * build the header for the HTTP Response
 * also allocate the buffer for body data */
void
BuildHeader_upnphttp(struct upnphttp * h, int respcode,
                     const char * respmsg,
                     int bodylen);

/* BuildResp_upnphttp() 
 * fill the res_buf buffer with the complete
 * HTTP 200 OK response from the body passed as argument */
void
BuildResp_upnphttp(struct upnphttp *, const char *, int);

/* BuildResp2_upnphttp()
 * same but with given response code/message */
void
BuildResp2_upnphttp(struct upnphttp * h, int respcode,
                    const char * respmsg,
                    const char * body, int bodylen);

/* SendResp_upnphttp() */
void
SendResp_upnphttp(struct upnphttp *);

void Send501(struct upnphttp *);

struct http_desc{
		char *path;
		char* (*gen_desc)(int *);
	};

extern struct http_desc *upnp_http_desc;

#endif

