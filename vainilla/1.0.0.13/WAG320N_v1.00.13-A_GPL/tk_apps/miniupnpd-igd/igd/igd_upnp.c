/*******************************************************
 *              WAG200GV2 igd_upnpd.  
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

/* unix sockets */
#include "maco.h"
#include "daemonize.h"
#include "upnpdescgen.h"
#include "upnphttp.h"
#include "upnpsoap.h"
#include "upnpevent.h"
#include "upnphttp_func.h"
#include "minissdp.h"
#include "miniupnpd.h"
#include "options.h"
#include "igd_descgen.h"
#include "igd_eventxml.h"
#include "igd_globalvars.h"
#include "igd_path.h"
#include "igd_soap.h"
#include "port.h"

/* MAX_ADD_LISTEN_ADDR : maximum number of interfaces
 * to listen to SSDP traffic */
#define MAX_ADD_LISTEN_ADDR (4)

char uuid_igd[] = "fc4ec57e-28b1-11db-88f8-a16830956233";
char uuid_wand[] = "fc4ec57e-2753-11db-88f8-0060085db3f6";
char uuid_land[] = "fc4ec57e-ba12-11db-88f8-a72469cbac1a";
char uuid_wancd[] = "fc4ec57e-092a-11db-88f8-0578ab52457c";

/* root Description of the UPnP Device 
 * fixed to match UPnP_IGD_InternetGatewayDevice 1.0.pdf */
static struct XMLElt rootDesc[] =
{
    /* 0 */
    {"IPADDR", lan_ipaddr},
    {"UPNP_PORT", lan_port},
    {"UUID_IGD", uuid_igd},
    {"UUID_WAND", uuid_wand},
    {"UUID_WANCD", uuid_wancd},
    {"UUID_LAND", uuid_land},
    {"HW_ID", upc},	/* required */
    {"SERIAL_NUMBER", serialNumber},
    {NULL, NULL}
};

struct event_list EVENT_URL_LIST[] = {
    { WANCFG_EVENTURL, check_wancfg, gen_wancfg_event_xml},
    { WANIPC_EVENTURL, check_wanip, gen_wanip_event_xml},
    { WANPPPC_EVENTURL, check_wanppp, gen_wanppp_event_xml},
    { LAYER3F_EVENTURL, check_layer3, gen_layer3_event_xml},
    { WANEthLCfg_EVENTURL, check_wanEthCfg, gen_wanEthCfg_event_xml},
    { LANHCfgM_XML_PATH, check_lanHcfg, gen_lanHcfg_event_xml},
    { NULL, NULL }
};

static const struct service_type_uuid IPConnection_service_types[] =
{
    {"upnp:rootdevice",uuid_igd,ROOT_DEVICE},
    {"urn:schemas-upnp-org:device:InternetGatewayDevice:1",uuid_igd,ROOT_DEVICE},
    {"urn:schemas-upnp-org:device:WANConnectionDevice:1",uuid_wancd,EMBED_DEVICE},
    {"urn:schemas-upnp-org:device:WANDevice:1",uuid_wand,EMBED_DEVICE},
    {"urn:schemas-upnp-org:device:LANDevice:1",uuid_land,EMBED_DEVICE},
    {"urn:schemas-upnp-org:service:Layer3Forwarding:1",uuid_igd,SERVICE},
    {"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",uuid_wand,SERVICE},
    {"urn:schemas-upnp-org:service:WANEthernetLinkConfig:1",uuid_wancd,SERVICE},
    {"urn:schemas-upnp-org:service:WANIPConnection:1",uuid_wancd,SERVICE},
    {"urn:schemas-upnp-org:service:LANHostConfigManagement:1",uuid_land,SERVICE},
    {NULL,NULL,0}
};

static const struct service_type_uuid PPPConnection_service_types[] =
{
    {"upnp:rootdevice",uuid_igd,ROOT_DEVICE},
    {"urn:schemas-upnp-org:device:InternetGatewayDevice:1",uuid_igd,EMBED_DEVICE},
    {"urn:schemas-upnp-org:device:WANConnectionDevice:1",uuid_wancd,EMBED_DEVICE},
    {"urn:schemas-upnp-org:device:WANDevice:1",uuid_wand,EMBED_DEVICE},
    {"urn:schemas-upnp-org:device:LANDevice:1",uuid_land,EMBED_DEVICE},
    {"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1",uuid_wand,SERVICE},
    {"urn:schemas-upnp-org:service:WANEthernetLinkConfig:1",uuid_wancd,SERVICE},
    {"urn:schemas-upnp-org:service:WANPPPConnection:1",uuid_wancd,SERVICE},
    {"urn:schemas-upnp-org:service:Layer3Forwarding:1",uuid_igd,SERVICE},
    {"urn:schemas-upnp-org:service:LANHostConfigManagement:1",uuid_land,SERVICE},
    {NULL,NULL,0}
};

/* Oliver add 2007.7.4*/
#ifdef DEBUG
void dump_config()
{
    printf("%s[%d] : dump config file : \n",__FILE__,__LINE__);
    printf("		");
    printf("ext_if_name=%s\n",ext_if_name);
    printf("		");
    printf("use_ext_ip_addr=%s\n",use_ext_ip_addr);
    printf("		");
    printf("lan_ipaddr=%s\n",lan_ipaddr);
    printf("		");
    printf("upstream_bitrate=%ld\n",upstream_bitrate);
    printf("		");
    printf("downstream_bitrate=%ld\n",downstream_bitrate);
    printf("		");
    printf("sysuptime=%d\n",sysuptime);
    printf("		");
    printf("logpackets=%d\n",logpackets);
    printf("		");
    printf("uuidvalue=%s\n",igd_uuidvalue);
    printf("		");
    printf("nat=%d\n",nat_enable);
    printf("		");
    printf("wan_type=%d\n",wan_type);
    printf("		");
    printf("pid_file=%s\n",pidfilename);
}
#endif

    static int
init(int argc, char * * argv, struct runtime_vars * v)
{
    int i;
    FILE *fp;
    char buf[512];
    char name[512],value[512];
    static char * optionsfile = "/etc/miniupnpd.conf";

    /* first check if "-f" option is used */
    for(i=2; i<argc; i++)
    {
        if(0 == strcmp(argv[i-1], "-f"))
        {
            optionsfile = argv[i];
            break;
        }
    }

    /* set initial values */
    v->n_add_listen_addr = 0;
    v->port = -1;
    v->notify_interval = 30;	/* seconds between SSDP announces */

    if((fp = fopen(optionsfile,"r")) == NULL)
    {
        printf("%s[%d] : open config file failed\n",__FUNCTION__,__LINE__);
        exit(1);
    }

    while(fgets(buf,sizeof(buf) - 1,fp) != NULL)
    {
        int i;
        if(buf[0] == '#')
            continue;
        memset(name,0,sizeof(name));
        memset(value,0,sizeof(value));
        divide(buf,name,value);

        i = -1;
        while(optionids[++i].id != MAX_TYPE)
        {
            if(strcmp(optionids[i].value,name) == 0)
                break;
        }

        if(optionids[i].id == MAX_TYPE)
        {
            //printf("%s[%d] : find an unknown option,ignor it\n",__FUNCTION__,__LINE__);
            continue;
        }

        switch (optionids[i].id){
            case UPNPEXT_IFNAME:
                strncpy(ext_if_name,value,NAME_MAX_LEN - 1);
                break;
            case UPNPEXT_IP:
                strncpy(use_ext_ip_addr,value,MAX_IP_ADDR - 1);
                break;
            case UPNPLISTENING_IP:
                strncpy(lan_ipaddr,value,MAX_IP_ADDR - 1);
                break;
            case UPNPPORT:
                strncpy(lan_port,value,NAME_MAX_LEN - 1);
                break;
            case UPNPBITRATE_UP:
                upstream_bitrate = atol(value);
                break;
            case UPNPBITRATE_DOWN:
                downstream_bitrate = atol(value);
                break;
            case UPNPNOTIFY_INTERVAL:
                interval = atoi(value);
                break;
            case UPNPSYSTEM_UPTIME:
                if(strcmp("yes",value) == 0)
                    sysuptime = 1;
                break;
            case UPNPPACKET_LOG:
                if(strcmp("yes",value) == 0)
                    logpackets= 1;
                break;
            case UPNPUUID:
                sprintf(igd_uuidvalue,"%s%s","uuid:",value);
                break;
            case NAT_ENABLE:
                nat_enable = atoi(value);
                break;
            case WAN_TYPE:
                if(strcmp("ppp",value) == 0)
                    wan_type = 1;
                break;
            case PID_FILE:
                strncpy(pidfilename,value,128 -1);
                break;
            case UPC:
                strncpy(upc,value,NAME_MAX_LEN -1);
                break;
	    case SERIAL_NUMBER:
		print2console("SERIAL_NUMBER: %s\n", value);
                strncpy(serialNumber,value,NAME_MAX_LEN -1);
		break;
            default :
                break;
        }
    }

    v->notify_interval = interval;
    v->port = atoi(lan_port);
    strncpy(v->listen_addr,lan_ipaddr,MAX_IP_ADDR - 1);

#ifdef DEBUG
    dump_config();
#endif

    return 0;
}

/* === main === */
/* process HTTP or SSDP requests */
    int
main(int argc, char * * argv)
{
    struct runtime_vars v;
    static struct  service_type_uuid *igd_service_types;

    if(init(argc, argv, &v) != 0)
        return 1;

    if(wan_type == 1)
        gen_root_xml(PPP_XML_MOD,ROOT_XML_PATH,rootDesc);
    else
        gen_root_xml(IP_XML_MOD,ROOT_XML_PATH,rootDesc);

    if(wan_type == 0)/* is ip connection*/
        igd_service_types = (struct service_type_uuid *)IPConnection_service_types;
    else
    {
        if(wan_type == 1)
            igd_service_types = (struct service_type_uuid *)PPPConnection_service_types;
        else
        {
            printf("Do not know connection type\n");
            exit(1);
        }
    };

    upnp_init(igd_service_types,pidfilename,igd_uuidvalue,ROOTDESC_PATH,igd_soapMethods,igd_http_desc,1);
    miniupnp_deamon(&v,EVENT_URL_LIST);

    if(unlink(pidfilename) < 0)
    {
        syslog(LOG_ERR, "Failed to remove pidfile %s: %m", pidfilename);
    }

    return 0;
}


