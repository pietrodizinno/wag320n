/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file generate device xml and service xml.
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "maco.h"
#include "upnphttp.h"
#include "upnpevent.h"
#include "upnphttp_func.h"
#include "upnpdescgen.h"
#include "igd_descgen.h"
#include "igd_path.h"
#include "igd_globalvars.h"

/* genRootDesc() :
 * - Generate the root description of the UPnP device.
 * - the len argument is used to return the length of
 *   the returned string. 
 * - tmp_uuid argument is used to build the uuid string */
char *
genRootDesc(int * len)
{
	return genServiceDesc_xml(len,ROOT_XML_PATH);
}

/* genWANIPCn() :
 * Generate the WANIPConnection xml description */
char *
genWANIPCn(int * len)
{
	return genServiceDesc_xml(len, WANIPC_XML_PATH);
}

/* genWANIPCn() :
 * Generate the WANIPConnection xml description */
char *
genWANPPPCn(int * len)
{
	return genServiceDesc_xml(len, WANPPPC_XML_PATH);
}

/* genWANCfg() :
 * Generate the WANInterfaceConfig xml description. */
char *
genWANCfg(int * len)
{
	return genServiceDesc_xml(len, WANCFG_XML_PATH);
}

char *genLayer3F(int * len)
{
	return genServiceDesc_xml(len, LAYER3F_XML_PATH);
}

char *genWANEthLCfg(int * len)
{
	return genServiceDesc_xml(len, WANEthLCfg_XML_ATH);	
}

char *genLANHCfgM(int * len)
{
	return genServiceDesc_xml(len, LANHCfgM_XML_PATH);
}

struct  http_desc igd_http_desc[]={
		{ROOTDESC_PATH,genRootDesc},
		{WANIPC_PATH,genWANIPCn},
		{WANPPPC_PATH,genWANPPPCn},
		{WANCFG_PATH,genWANCfg},
		{LAYER3F_PATH,genLayer3F},
		{WANEthLCfg_PATH,genWANEthLCfg},
		{LANHCfgM_PATH,genLANHCfgM},
		{NULL,NULL}
};

