/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file read var from config file.
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "options.h"

struct option optionids[] = {
	{ UPNPEXT_IFNAME, "ext_ifname" },
	{ UPNPEXT_IP,	"ext_ip" },
	{ UPNPLISTENING_IP, "listening_ip" },
	{ UPNPPORT, "port" },
	{ UPNPBITRATE_UP, "bitrate_up" },
	{ UPNPBITRATE_DOWN, "bitrate_down" },
	{ UPNPNOTIFY_INTERVAL, "notify_interval" },
	{ UPNPSYSTEM_UPTIME, "system_uptime" },
	{ UPNPPACKET_LOG, "packet_log" },
	{ UPNPUUID, "uuid"},
	{ NAT_ENABLE,"nat"},
	{ WAN_TYPE,"wan_type"},
	{ PID_FILE,"pid_file"},
	{ UPC,"upc"},
	{ SERIAL_NUMBER, "serial"},
	{ MAX_TYPE,""}
};

int divide(char *buf, char *name, char *value)
{
	int i = -1;
	int j = 0;
	int find = 0;
		
	while(buf[++i] != 0)
	{
		if(buf[i] == '=')
		{
			find = 1;
			continue;
		}
		if(find == 0)
			name[i] = buf[i];
		else
			value[j++] = buf[i];
	}
	if(value[j-1] == '\r' || value[j-1] == '\n')
		value[j-1] = 0;
	if(value[j-2] == '\r' || value[j-2] == '\n')
		value[j-2] = 0;
	return find;
}
