/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file generate event xml
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/
 
#ifndef __H_IGDEVENTXML
#define __H_IGDEVENTXML

int gen_wancfg_event_xml(char *);

int gen_wanip_event_xml(char *);

int gen_wanppp_event_xml(char *);

int gen_layer3_event_xml(char *);

int gen_wanEthCfg_event_xml(char *);

int gen_lanHcfg_event_xml(char *);

#endif

