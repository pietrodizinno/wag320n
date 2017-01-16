/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file generate event  xml.
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/file.h>
#include <syslog.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#include "maco.h"
#include "upnphttp.h"
#include "upnpevent.h"
#include "upnphttp_func.h"
#include "getifaddr.h"
#include "igd_globalvars.h"
#include "igd_descgen.h"
#include "igd_path.h"
#include "daemonize.h"
#include "upnpsoap.h"
#include "options.h"
#include "minissdp.h"
#include "port.h"
#include "igd_eventxml.h"


struct event_table wancfg_event_table[] = {
			{"PhysicalLinkStatus",get_EthernetLinkStatus},
			{NULL,NULL}
	};

struct event_table wanip_event_table[] = {
			{"PossibleConnectionTypes",get_connectType},
			{"ConnectionStatus",get_connectStatus},
			{"X_Name",get_XName},
			{"ExternalIPAddress",get_Exip},
			{"PortMappingNumberOfEntries",get_MappNum},
			{NULL,NULL}
	};

struct event_table wanppp_event_table[] = {
			{"PossibleConnectionTypes",get_connectType},
			{"ConnectionStatus",get_connectStatus},
			{"X_Name",get_XName},
			{"ExternalIPAddress",get_Exip},
			{"PortMappingNumberOfEntries",get_MappNum},
			{NULL,NULL}
	};

struct event_table layer3_event_table[] = {
			{"DefaultConnectionService",get_conserver},
			{NULL,NULL}
	};

struct event_table wanEthCfg_event_table[] = {
			{"EthernetLinkStatus",get_EthernetLinkStatus},
			{NULL,NULL}
	};

struct event_table lanHcfg_event_table[] = {
			{NULL,NULL}
	};

static char event_head[] = "<e:propertyset xmlns:e=\"urn:schemas-upnp-org:event-1-0\">\r\n";
static char event_tail[] = "</e:propertyset>\r\n";

static int add_property(char *buf, struct event_table *table)
{
	char tmp[128];
	
	table->get_value(tmp);
	return sprintf(buf,"<e:property>\r\n<%s>%s</%s></e:property>\r\n",
		table->name, tmp, table->name);
}

int gen_wancfg_event_xml(char *buf)
{
	int i = 0;
	int j = -1;

	i += sprintf(buf,event_head);
	while(wancfg_event_table[++j].name != NULL)	
	{
		i += add_property(buf+i, &wancfg_event_table[j]);
	}
	i += sprintf(buf + i,"%s",event_tail);
	return i;
}

int gen_wanip_event_xml(char *buf)
{
	int i = 0;
	int j = -1;

	i += sprintf(buf,event_head);
	while(wanip_event_table[++j].name != NULL)	
	{
		i += add_property(buf+i, &wanip_event_table[j]);
	}
	i += sprintf(buf + i,"%s",event_tail);
	return i;
}

int gen_wanppp_event_xml(char *buf)
{
	int i = 0;
	int j = -1;

	i += sprintf(buf,event_head);
	while(wanppp_event_table[++j].name != NULL)	
	{
		i += add_property(buf+i, &wanppp_event_table[j]);
	}
	i += sprintf(buf + i,"%s",event_tail);
	return i;
}

int gen_layer3_event_xml(char *buf)
{
	int i = 0;
	int j = -1;

	i += sprintf(buf,event_head);
	while(layer3_event_table[++j].name != NULL)	
	{
		i += add_property(buf+i, &layer3_event_table[j]);
	}
	i += sprintf(buf + i,"%s",event_tail);
	return i;
}

int gen_wanEthCfg_event_xml(char *buf)
{
	int i = 0;
	int j = -1;

	i += sprintf(buf,event_head);
	while(wanEthCfg_event_table[++j].name != NULL)	
	{
		i += add_property(buf+i, &wanEthCfg_event_table[j]);
	}
	i += sprintf(buf + i,"%s",event_tail);
	return i;
}

int gen_lanHcfg_event_xml(char *buf)
{
	int i = 0;
	int j = -1;

	i += sprintf(buf,event_head);
	while(lanHcfg_event_table[++j].name != NULL)	
	{
		i += add_property(buf+i, &lanHcfg_event_table[j]);
	}
	i += sprintf(buf + i,"%s",event_tail);
	return i;
}


