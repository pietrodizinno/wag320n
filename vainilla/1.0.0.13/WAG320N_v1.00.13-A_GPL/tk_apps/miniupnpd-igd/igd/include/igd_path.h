/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file define device, service and event path.
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#ifndef __IGDPATH_H__
#define __IGDPATH_H__

/* Paths and other URLs in the miniupnpd http server */

///#define ROOTDESC_PATH 		"/rootDesc.xml"
#define ROOTDESC_PATH           "/gateway.xml"


#define DUMMY_PATH			"/dummy.xml"

#define WANCFG_PATH			"/cmnicfg.xml"
#define WANCFG_CONTROLURL	"/upnp/control/WANCommonIFC1"
#define WANCFG_EVENTURL		"/upnp/event/WANCommonIFC1"
#define WANCFG_XML_PATH			"/usr/upnp/cmnicfg.xml"

#define WANIPC_PATH			"/ipcfg.xml"
#define WANIPC_CONTROLURL	"/upnp/control/WANIPConn1"
#define WANIPC_EVENTURL		"/upnp/event/WANIPConn1"
#define WANIPC_XML_PATH			"/usr/upnp/ipcfg.xml"

#define WANPPPC_PATH			"/pppcfg.xml"
#define WANPPPC_CONTROLURL	"/upnp/control/WANPPPConn1"
#define WANPPPC_EVENTURL		"/upnp/event/WANPPPConn1"
#define WANPPPC_XML_PATH			"/usr/upnp/pppcfg.xml"

#define LAYER3F_PATH "/l3frwd.xml"
#define LAYER3F_CONTROLURL "/upnp/control/L3Forwarding1"
#define LAYER3F_EVENTURL "/upnp/event/L3Forwarding1"
#define LAYER3F_XML_PATH "/usr/upnp/l3frwd.xml"

#define WANEthLCfg_PATH  "/wanelcfg.xml"
#define WANEthLCfg_CONTROLURL  "/upnp/control/WANEthLinkC1"
#define WANEthLCfg_EVENTURL  "/upnp/event/WANEthLinkC1"
#define WANEthLCfg_XML_ATH  "/usr/upnp/wanelcfg.xml"

#define LANHCfgM_PATH  "/lanhostc.xml"
#define LANHCfgM_CONTROLURL  "/upnp/control/LANHostCfg1"
#define LANHCfgM_EVENTURL  "/upnp/control/LANHostCfg1"
#define LANHCfgM_XML_PATH  "/usr/upnp/lanhostc.xml"

#define PPP_XML_MOD "/usr/upnp/gateway_ppp.mod"
#define IP_XML_MOD "/usr/upnp/gateway.mod"
#define ROOT_XML_PATH "/tmp/gateway.xml"

#endif

