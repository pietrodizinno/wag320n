/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file read var from config file.
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

/* enum of option available in the miniupnpd.conf */
enum upnpconfigoptions {
	UPNP_INVALID = 0,
	UPNPEXT_IFNAME,
	UPNPEXT_IP,
	UPNPLISTENING_IP,
	UPNPPORT,
	UPNPBITRATE_UP,
	UPNPBITRATE_DOWN,
	UPNPNOTIFY_INTERVAL,
	UPNPSYSTEM_UPTIME, 
	UPNPPACKET_LOG,
	UPNPUUID, 
	NAT_ENABLE,
	WAN_TYPE,
	PID_FILE,
	UPC,
	SERIAL_NUMBER,
	MAX_TYPE
};

#define MAX_OPTION_VALUE_LEN (80)
struct option
{
	enum upnpconfigoptions id;
	char value[MAX_OPTION_VALUE_LEN];
};

int divide(char *, char *, char *);

extern struct option optionids[];

#endif

