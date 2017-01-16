/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file define global var.
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#ifndef __IGDGLOBALVARS_H__
#define __IGDGLOBALVARS_H__

/* Add for addition option */
#define DESC_MAX_LEN 128
#define NAME_MAX_LEN 64
#define MAX_IP_ADDR (20)

extern char ext_if_name[];

/* forced ip address to use for this interface
 * when NULL, getifaddr() is used */
extern char use_ext_ip_addr[];

extern unsigned long downstream_bitrate;
extern unsigned long upstream_bitrate;

/* use system uptime */
extern int sysuptime;

/* log packets flag */
extern int logpackets;

extern char pidfilename[];

extern char igd_uuidvalue[];

extern char lan_port[];
extern char lan_ipaddr[];
extern char upc[];
extern char serialNumber[];
extern int interval;
extern int nat_enable;
extern int wan_type;/* default to set ip mode */
/* UPnP permission rules : */
extern struct upnpperm * upnppermlist;
extern unsigned int num_upnpperm;
#endif

