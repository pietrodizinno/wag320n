/* $Id: port.h,v 1.1.1.1 2009-01-05 09:01:14 fred_fu Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * author: Ryan Wagoner
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution 
 * This file Create by Oliver for portting to other system */

#ifndef __PORT_H__
#define __PORT_H__

int get_wan_up();

int start_wan();

int  stop_wan();

unsigned int get_uptime();

int check_wancfg();

int check_wanip();

int check_wanppp();

int check_layer3();

int check_wanEthCfg();

int check_lanHcfg();

int get_EthernetLinkStatus(char *);

int get_connectType(char *);
 
int get_connectStatus(char *);

int get_XName(char *);

int get_Exip(char *);

int get_MappNum(char *);

int get_conserver(char *);

int get_EthernetLinkStatus(char *);

int get_connectType(char *);
 
int get_connectStatus(char *);

int get_XName(char *);

int get_Exip(char *);

int get_MappNum(char *);

int get_conserver(char *);

#endif

