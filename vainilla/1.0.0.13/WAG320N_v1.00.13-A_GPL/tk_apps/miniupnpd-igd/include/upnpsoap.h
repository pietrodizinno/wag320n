/* $Id: upnpsoap.h,v 1.1.1.1 2009-01-05 09:01:14 fred_fu Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#ifndef __UPNPSOAP_H__
#define __UPNPSOAP_H__

struct method
{
	const char * methodName; 
	void (*methodImpl)(struct upnphttp *);
} ;

extern struct method *soapMethods;

/* ExecuteSoapAction():
 * this method executes the requested Soap Action */
void
ExecuteSoapAction(struct upnphttp *, const char *, int);

/* SoapError():
 * sends a correct SOAP error with an UPNPError code and 
 * description */
void
SoapError(struct upnphttp * h, int errCode, const char * errDesc);

void
BuildSendAndCloseSoapResp(struct upnphttp * , const char * , int );

#endif

