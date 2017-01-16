/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file define global var.
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#include <sys/types.h>
#include <netinet/in.h>

#include "igd_globalvars.h"

/* network interface for internet */
char ext_if_name[NAME_MAX_LEN];

/* forced ip address to use for this interface
 * when NULL, getifaddr() is used */
char use_ext_ip_addr[MAX_IP_ADDR];

unsigned long downstream_bitrate = 0;
unsigned long upstream_bitrate = 0;

/* use system uptime */
int sysuptime = 0;

/* log packets flag */
int logpackets = 0;

char pidfilename[128] = "/var/run/igd_miniupnpd.pid";

char igd_uuidvalue[64] = "uuid:00000000-0000-0000-0000-000000000000";

char lan_port[NAME_MAX_LEN];
char lan_ipaddr[NAME_MAX_LEN];
char upc[NAME_MAX_LEN];
char serialNumber[NAME_MAX_LEN];

int interval = 30;
int nat_enable = 1;
int wan_type = 0;/* default to set ip mode */
/* UPnP permission rules : */
struct upnpperm * upnppermlist = 0;
unsigned int num_upnpperm = 0;

