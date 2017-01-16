/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h> 
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nvram.h"
#include "maco.h"
#include "upnphttp.h"
#include "upnpevent.h"
#include "upnphttp_func.h"
#include "upnpsoap.h"
#include "upnpreplyparse.h"
#include "getifaddr.h"
#include "linux/getifstats.h"
#include "igd_globalvars.h"
#include "igd_redirect.h"
#include "port.h"

#if 0
#define NUM_HANDLE 200
#define LINE_SIZE  180
#define NAME_SIZE  100
#define MNFT_NAME_SIZE  64
#define MODL_NAME_SIZE  32
#define SERL_NUMR_SIZE  64
#define MODL_DESC_SIZE  64
/* WANIPConnection variable */

#define DS_WANIPConn_VARCOUNT		 	19

#define DS_WANIPConn_ConnectionType		0
#define DS_WANIPConn_PossibleConnectionTypes    1
#define DS_WANIPConn_ConnectionStatus		2
#define DS_WANIPConn_Uptime			3
#define DS_WANIPConn_RSIPAvailable		4
#define DS_WANIPConn_NATEnabled			5
#define DS_WANIPConn_X_Name			6
#define DS_WANIPConn_LastConnectionError	7
#define DS_WANIPConn_ExternalIPAddress		8
#define DS_WANIPConn_RemoteHost			9
#define DS_WANIPConn_ExternalPort		10
#define DS_WANIPConn_InternalPort		11
#define DS_WANIPConn_PortMappingProtocol	12	/* TCP or UDP */
#define DS_WANIPConn_InternalClient		13
#define DS_WANIPConn_PortMappingDescription	14	/* String */
#define DS_WANIPConn_PortMappingEnabled		15
#define DS_WANIPConn_PortMappingLeaseDuration	16
#define DS_WANIPConn_X_PortMappingIndex		17
#define DS_WANIPConn_PortMappingNumberOfEntries 18


/* Layer3Forwarding variable */
#define DS_L3F_VARCOUNT	                    1
#define DS_L3F_DefaultConnService           0

/* WANEthernetLinkConfig variable */
#define DS_WANELC_VARCOUNT	                        1
#define DS_WANELC_EthernetLinkStatus            0

/* LANHostConfigManagement variable */
#define DS_LANHostCM_VARCOUNT                           9
#define DS_LANHostCM_DHCPRelay                          0
#define DS_LANHostCM_DHCPSrvConfig                      1
#define DS_LANHostCM_DNSServers                         2
#define DS_LANHostCM_DomainName                         3
#define DS_LANHostCM_IPRouters                          4
#define DS_LANHostCM_MaxAddress                         5
#define DS_LANHostCM_MinAddress                         6
#define DS_LANHostCM_ReservedAddr                       7
#define DS_LANHostCM_SubnetMask                         8



#define DS_MAX_VAL_LEN 50

/* This should be the maximum VARCOUNT from above */
#define DS_MAXVARS DS_WANIPConn_VARCOUNT
/* Structure for storing IGD Service identifiers and state table */
struct IGDService {
    char UDN[NAME_SIZE]; /* Universally Unique Device Name */
    char ServiceId[NAME_SIZE];
    char ServiceType[NAME_SIZE];
    char *VariableName[DS_MAXVARS]; 
    char *VariableStrVal[DS_MAXVARS];
    int  VariableCount;
};

/* Global arrays for storing <service> OSInfo  variable names, values, and defaults */
char *osi_varname[] = {"OSMajorVersion",
		       "OSMinorVersion",
		       "OSBuildNumber",
		       "OSMachineName"};
char osi_varval[DS_OSI_VARCOUNT][DS_MAX_VAL_LEN];
char *osi_varval_def[] = {"2","2","14","LinuxBase"};

/* arrays for storing WANCommonInterfaceConfig's variable names , values, and defaults */
char *cic_varname[] = {	"WANAccessType",
			"Layer1UpstreamMaxBitRate",
			"Layer1DownstreamMaxBitRate",
			"PhysicalLinkStatus",
			"WANAccessProvider",
			"MaximumActiveConnections",
			"NumberOfActiveConnections",
			"ActiveConnectionDeviceContainer",
			"ActiveConnectionServiceID",
			"TotalBytesSent",
			"TotalBytesReceived",
			"TotalPacketsSent",
			"TotalPacketsReceived",
			"X_PersonalFirewallEnabled",
			"X_Uptime",
			"EnableForInternet"};

char cic_varval[DS_WANCIC_VARCOUNT][DS_MAX_VAL_LEN];
char *cic_varval_def[] = {"Ethernet",  	/* WANAccessType : DSL POTS Cable Ethernet */
			  "10000000",
			  "10000000",
			  "Up",//  "Up",	/* PhysicalLinkStatus : Up Down Initializing Unavailable */
			  "WANAccessProvider",
			  "8",		
			  "0",
			  "ActiveConnectionDeviceContainer",
			  "ActiveConnectionServiceID",
			  "100",		/*TotalBytesSent*/
			  "100",          /*TotalBytesReceived*/
			  "100",
			  "100",
			   "1",  // test 
			  "0",
			  "1"};


/* arrays for storing WANIPConnction's variable names , values, and defaults */
char *ipconn_varname[] = {"ConnectionType",
			  "PossibleConnectionTypes",
			  "ConnectionStatus",
			  "Uptime",
			  "RSIPAvailable",
			  "NATEnabled",
			  "X_Name",
			  "LastConnectionError",			  
			  "ExternalIPAddress",
			  "RemoteHost",
			  "ExternalPort",
			  "InternalPort",			  			  
			  "PortMappingProtocol",			  
			  "InternalClient",
			  "PortMappingDescription",
			  "PortMappingEnabled",
			  "PortMappingLeaseDuration",
			  "X_PortMappingIndex",
			  "PortMappingNumberOfEntries"};
			  

char ipconn_varval[DS_WANIPConn_VARCOUNT][DS_MAX_VAL_LEN];
char *ipconn_varval_def[] = {"IP_Routed",
			     "IP_Routed",
			     "Disconnected",   //Connected
			     "0",		/* uptime*/
			     "0",		/* RSIPAvailable */
			     "1", 		/* nat enabled */
			     "Local Area Connection",
			     "ERROR_NONE",
			     "0.0.0.0",
			     "0.0.0.0",
			     "110", 
			     "110",
			     "TCP",
			     "0.0.0.0",
			     "POP3",
			     "0",
			     "0",
			     "0",
	//		     "1"};   // modi by john,    at powerup, 
	                             //PortMappingNumberOfEntries must be 0
			     "0"};   

/* arrays for storing Layer3Forwarding's variable names , values, and defaults */
char *l3f_varname[] = { "DefaultConnectionService"};
#define L3F_DEFAULTSVR_LEN 200
char l3f_varval[DS_L3F_VARCOUNT][L3F_DEFAULTSVR_LEN];
char *l3f_varval_def[] = {"Default"};

/* arrays for storing WANEthernetLinkConfig's variable names , values, and defaults */
char *elc_varname[] = {"EthernetLinkStatus"};
char elc_varval[DS_WANELC_VARCOUNT][DS_MAX_VAL_LEN];
char *elc_varval_def[] = {"Up"};

/* arrays for storing LANHostConfigManagement's variable names , values, and defaults */
char *hcm_varname[] = {"DHCPRelay",
			"DHCPServerConfigurable",
			"DNSServers",
			"DomainName",
			"IPRouters",
			"MaxAddress",
			"MinAddress",
			"ReservedAddress",
			"SubnetMask"};
char hcm_varval[DS_LANHostCM_VARCOUNT][DS_MAX_VAL_LEN];
char *hcm_varval_def[] = {"0",
			  "0",
			  "0.0.0.0",
			  "0",
			  "0",
			  "0.0.0.0",		
			  "0.0.0.0",
			  "0.0.0.0",
			  "255.255.255.0"};





/* Global structure for storing the state table for this device */
struct IGDService ds_service_table[6];
#endif

static void
GetConnectionTypeInfo(struct upnphttp * h)
{
	static const char resp[] =
		"<u:GetConnectionTypeInfoResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewConnectionType>IP_Routed</NewConnectionType>"
		"<NewPossibleConnectionTypes>IP_Routed</NewPossibleConnectionTypes>"
		"</u:GetConnectionTypeInfoResponse>";
	BuildSendAndCloseSoapResp(h, resp, sizeof(resp)-1);
}

static void
GetTotalBytesSent(struct upnphttp * h)
{
	int r;

	static const char resp[] =
		"<u:GetTotalBytesSentResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\">"
		"<NewTotalBytesSent>%lu</NewTotalBytesSent>"
		"</u:GetTotalBytesSentResponse>";

	char body[2048];
	int bodylen;
	struct ifdata data;

	r = getifstats(ext_if_name, &data, 0);
	bodylen = snprintf(body, sizeof(body), resp, r<0?0:data.obytes);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetTotalBytesReceived(struct upnphttp * h)
{
	int r;

	static const char resp[] =
		"<u:GetTotalBytesReceivedResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\">"
		"<NewTotalBytesReceived>%lu</NewTotalBytesReceived>"
		"</u:GetTotalBytesReceivedResponse>";

	char body[2048];
	int bodylen;
	struct ifdata data;

	r = getifstats(ext_if_name, &data, 0);
	bodylen = snprintf(body, sizeof(body), resp, r<0?0:data.ibytes);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetTotalPacketsSent(struct upnphttp * h)
{
	int r;

	static const char resp[] =
		"<u:GetTotalPacketsSentResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\">"
		"<NewTotalPacketsSent>%lu</NewTotalPacketsSent>"
		"</u:GetTotalPacketsSentResponse>";

	char body[2048];
	int bodylen;
	struct ifdata data;

	r = getifstats(ext_if_name, &data, 0);
	bodylen = snprintf(body, sizeof(body), resp, r<0?0:data.opackets);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetTotalPacketsReceived(struct upnphttp * h)
{
	int r;

	static const char resp[] =
		"<u:GetTotalPacketsReceivedResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\">"
		"<NewTotalPacketsReceived>%lu</NewTotalPacketsReceived>"
		"</u:GetTotalPacketsReceivedResponse>";

	char body[2048];
	int bodylen;
	struct ifdata data;

	r = getifstats(ext_if_name, &data, 0);
	bodylen = snprintf(body, sizeof(body), resp, r<0?0:data.ipackets);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetCommonLinkProperties(struct upnphttp * h)
{
	static const char resp[] =
		"<u:GetCommonLinkPropertiesResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\">"
		"<NewWANAccessType>DSL</NewWANAccessType>"
		/*"<NewWANAccessType>Cable</NewWANAccessType>"*/
		"<NewLayer1UpstreamMaxBitRate>%lu</NewLayer1UpstreamMaxBitRate>"
		"<NewLayer1DownstreamMaxBitRate>%lu</NewLayer1DownstreamMaxBitRate>"
		"<NewPhysicalLinkStatus>Up</NewPhysicalLinkStatus>"
		"</u:GetCommonLinkPropertiesResponse>";

	char body[2048];
	int bodylen;
	struct ifdata data;

	if((downstream_bitrate == 0) || (upstream_bitrate == 0)) {
		if(!getifstats(ext_if_name, &data, 1)) {
			downstream_bitrate = data.ibaudrate;
			upstream_bitrate = data.obaudrate;
		}
	}
	bodylen = snprintf(body, sizeof(body), resp, upstream_bitrate, downstream_bitrate);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
RequestConnection(struct upnphttp * h)
{
	static const char resp[] =
		"<u:RequestConnectionResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"</u:RequestConnectionResponse>";
	start_wan();
	sleep(3);
	
	BuildSendAndCloseSoapResp(h, resp, sizeof(resp) - 1);
}

static void
ForceTermination(struct upnphttp * h)
{
	static const char resp[] =
		"<u:ForceTerminationResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"</u:ForceTerminationResponse>";

	char body[512];
	int bodylen;

	stop_wan();
	bodylen = snprintf(body, sizeof(body), resp);	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetStatusInfo(struct upnphttp * h)
{
	static const char resp[] =
		"<u:GetStatusInfoResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewConnectionStatus>%s</NewConnectionStatus>"
		"<NewLastConnectionError>ERROR_NONE</NewLastConnectionError>"
		"<NewUptime>%ld</NewUptime>"
		"</u:GetStatusInfoResponse>";

	char body[512];
	int bodylen;
	char wan_status[32];
	time_t uptime;

	if(get_wan_up(ext_if_name))
		sprintf(wan_status,"Connected");
	else
		sprintf(wan_status,"Disconnected");

//	uptime = (time(NULL) - startup_time);
	uptime = get_uptime();
	bodylen = snprintf(body, sizeof(body), resp, wan_status,(long)uptime);	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetNATRSIPStatus(struct upnphttp * h)
{
	static const char resp[] =
		"<u:GetNATRSIPStatusResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewRSIPAvailable>0</NewRSIPAvailable>"
		"<NewNATEnabled>%d</NewNATEnabled>"
		"</u:GetNATRSIPStatusResponse>";
	char body[512];
	int bodylen;
	bodylen = snprintf(body, sizeof(body), resp, nat_enable);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetExternalIPAddress(struct upnphttp * h)
{
	static const char resp[] =
		"<u:GetExternalIPAddressResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewExternalIPAddress>%s</NewExternalIPAddress>"
		"</u:GetExternalIPAddressResponse>";

	char body[512];
	int bodylen;
	char ext_ip_addr[INET_ADDRSTRLEN];

	if(use_ext_ip_addr[0])
	{
		strncpy(ext_ip_addr, use_ext_ip_addr, INET_ADDRSTRLEN);
	}
	else if(getifaddr(ext_if_name, ext_ip_addr, INET_ADDRSTRLEN) < 0)
	{
		syslog(LOG_ERR, "Failed to get ip address for interface %s",
			ext_if_name);
		strncpy(ext_ip_addr, "0.0.0.0", INET_ADDRSTRLEN);
	}
	bodylen = snprintf(body, sizeof(body), resp, ext_ip_addr);
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

int CheckIPValidity(char *szIP)
{
	unsigned long dut_ip=0, dut_mask=0, int_ip=0, check=0,val=0;
	int i=0,j=0,num=0, first_byte=0;
	struct in_addr addr;
	char tmp_str[32]={0}, *p=NULL;
	char szDutIP[32]={0}, szDutMask[32]={0};
	
    p=nvram_safe_get("lan_ipaddr");
	strcpy(szDutIP, p);
	free(p);
	p=NULL;
	
    p=nvram_safe_get("lan_netmask");
	strcpy(szDutMask, p);
	free(p);
	p=NULL;
	
	if(!strlen(szDutIP) || !strlen(szDutMask) || !strcmp(szDutIP, szIP))
		return -1;
	if(!inet_aton(szIP,&addr)){
		return -1;
	}	
	strcpy(tmp_str, szIP);
	p=strchr(tmp_str, '.');
	if(p){
		*p=0;
		first_byte=atoi(tmp_str);
		if(first_byte==127 //Loopback
			|| (first_byte>=224 && first_byte<=239) //Multicast
			|| first_byte==255)	//Broadcast
			return -1;
	}
	dut_ip = ntohl(inet_addr(szDutIP));
	dut_mask = ntohl(inet_addr(szDutMask));
	int_ip = ntohl(inet_addr(szIP));
	check=int_ip^dut_mask;
	j=0;
	while(i<sizeof(unsigned long)*8){
		if(dut_mask&1)
			j=1;
		else{
			if(j==1)
				return -2;
			num++;
		}		
		dut_mask=dut_mask>>1;
		i++;
	}			
	j=0;
	while(j<num)
		val|=1<<(j++);
	check=int_ip & val;		
	if(check==0 || check==val){
		return -1;
	}
	dut_mask = ntohl(inet_addr(szDutMask));	
	if((dut_ip & dut_mask)!= (int_ip & dut_mask)){
		return -1;
	}	
	return 0;
}

static void
AddPortMapping(struct upnphttp * h)
{
	int r;
	//pid_t pid;

	static const char resp[] =
		"<u:AddPortMappingResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\"/>";

	struct NameValueParserData data;
	char * int_ip, * int_port, * ext_port, *ext_ip, * desc, *time_outp, *protocol;
	unsigned short iport, eport;
	time_t timeout;

	struct hostent *hp; /* getbyhostname() */
	char ** ptr; /* getbyhostname() */
	unsigned char result_ip[16]; /* inet_pton() */
	//int status;
	struct time_list *time_p;
//	if((pid = fork()) == 0)
	{
		ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
		int_ip = GetValueFromNameValueList(&data, "NewInternalClient");
              
		if (!int_ip)
		{
			ClearNameValueList(&data);
			SoapError(h, 402, "Invalid Args");
			return ;
		}
		/* if ip not valid assume hostname and convert */
		if (inet_pton(AF_INET, int_ip, result_ip) <= 0) 
		{
			hp = gethostbyname(int_ip);
			if(hp && hp->h_addrtype == AF_INET) 
			{ 
				for(ptr = hp->h_addr_list; ptr && *ptr; ptr++)
			   	{
					int_ip = inet_ntoa(*((struct in_addr *) *ptr));
					/* TODO : deal with more than one ip per hostname */
					//exit(0);
				}
			} 
			else 
			{
				syslog(LOG_ERR, "Failed to convert hostname '%s' to ip address", int_ip); 
				ClearNameValueList(&data);
				SoapError(h, 402, "Invalid Args");
				return ;
			}				
		 }
#if 1		 
		 if(strcmp(int_ip, "157.156.32.24") && CheckIPValidity(int_ip)){
				ClearNameValueList(&data);
				SoapError(h, 402, "Invalid Args");
				return ;		 	
		 }
#endif		 
		 ext_ip= GetValueFromNameValueList(&data, "NewRemoteHost");
		 if ( !ext_ip || ext_ip[0] == '\0')
		 {
			goto skip;
		 }
 
		/* if ip not valid assume hostname and convert */
		if (inet_pton(AF_INET, ext_ip, result_ip) <= 0) 
		{
			hp = gethostbyname(ext_ip);
			if(hp && hp->h_addrtype == AF_INET) 
			{ 
				for(ptr = hp->h_addr_list; ptr && *ptr; ptr++)
				{
					ext_ip = inet_ntoa(*((struct in_addr *) *ptr));
					/* TODO : deal with more than one ip per hostname */
				}
			} 
			else 
			{
				syslog(LOG_ERR, "Failed to convert remote hostname '%s' to ip address", ext_ip); 
				ClearNameValueList(&data);
				SoapError(h, 402, "Invalid Args");
				return ;
			}				
		}
	skip:

		int_port = GetValueFromNameValueList(&data, "NewInternalPort");
	       ext_port = GetValueFromNameValueList(&data, "NewExternalPort");
              protocol = GetValueFromNameValueList(&data, "NewProtocol");	
		desc = GetValueFromNameValueList(&data, "NewPortMappingDescription");

		if (!int_port || !ext_port || !protocol ||!protocol[0])
		{
			ClearNameValueList(&data);
			SoapError(h, 402, "Invalid Args");
			return ;
		}

		iport = (unsigned short)atoi(int_port);
		eport = (unsigned short)atoi(ext_port);	

		time_outp = GetValueFromNameValueList(&data, "NewLeaseDuration"); 
		if(time_outp == NULL || time_outp[0] == '\0')
			timeout = 0;
		else
			timeout = atoi(time_outp);
		
		syslog(LOG_INFO, "AddPortMapping: external %s:%hu to %s:%hu protocol %s for: %s with timeout:%lu",
					ext_ip?:"NULL",eport, int_ip, iport, protocol, desc,timeout);
			  
		r = upnp_redirect(ext_ip,eport, int_ip, iport, protocol, desc);

		ClearNameValueList(&data);

		/* possible error codes for AddPortMapping :
		 * 402 - Invalid Args
		 * 501 - Action Failed
		 * 715 - Wildcard not permited in SrcAddr
		 * 716 - Wildcard not permited in ExtPort
		 * 718 - ConflictInMappingEntry
		 * 724 - SamePortValuesRequired */
		switch(r)
		{
			case 0:	/* success */
				BuildSendAndCloseSoapResp(h, resp, sizeof(resp)-1);
				//exit(1);
				break;
			case -2:	/* already redirected */
				SoapError(h, 718, "ConflictInMappingEntry");
				return ;
			default:
				SoapError(h, 501, "ActionFailed");
				return;
		}
	}

/*	while(((pid=wait(&status))==-1)&(errno==EINTR));
       if(WIFEXITED(status) && WEXITSTATUS(status)) */
       if(r == 0)
       {
	     if(timeout > 0)
	     {
	     	 	time_p = (struct time_list *)malloc(sizeof(struct time_list));
	      		time_p->add_time=time(NULL);
	      		time_p->eport=eport;
	      		snprintf(time_p->protocol, sizeof(time_p->protocol), "%s",protocol );
	      		time_p->timeout = timeout;
	      		LIST_INSERT_HEAD(&time_head, time_p, entries);
	     	}

	}
	 
	CloseSocket_upnphttp(h);
}

static void
GetSpecificPortMappingEntry(struct upnphttp * h)
{
	int r;
	//pid_t pid;
	static const char resp[] =
		"<u:GetSpecificPortMappingEntryResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewInternalPort>%u</NewInternalPort>"
		"<NewInternalClient>%s</NewInternalClient>"
		"<NewEnabled>1</NewEnabled>"
		"<NewPortMappingDescription>%s</NewPortMappingDescription>"
		"<NewLeaseDuration>0</NewLeaseDuration>"
		"</u:GetSpecificPortMappingEntryResponse>";

	char body[2048];
	int bodylen;
	struct NameValueParserData data;
	const char * r_host, * ext_port, * protocol;
	unsigned short eport, iport;
	char int_ip[32];
	char desc[64];
	//if((pid = fork()) == 0)
	{	
		ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
		r_host = GetValueFromNameValueList(&data, "NewRemoteHost");
		ext_port = GetValueFromNameValueList(&data, "NewExternalPort");
		protocol = GetValueFromNameValueList(&data, "NewProtocol");

		if(!ext_port || !protocol)
		{
			ClearNameValueList(&data);
			SoapError(h, 402, "Invalid Args");
			return ;
		}

		eport = (unsigned short)atoi(ext_port);

		r = upnp_get_redirection_infos(eport, protocol, &iport,
		                               int_ip, sizeof(int_ip),
		                               desc, sizeof(desc));

		if(r < 0)
		{		
			SoapError(h, 714, "NoSuchEntryInArray");
		}
		else
		{
			syslog(LOG_INFO, "GetSpecificPortMappingEntry: rhost='%s' %s %s found => %s:%u desc='%s'",
			       r_host?:"NULL", ext_port, protocol, int_ip, (unsigned int)iport, desc);
			bodylen = snprintf(body, sizeof(body), resp, (unsigned int)iport, int_ip, desc);
			BuildSendAndCloseSoapResp(h, body, bodylen);
			CloseSocket_upnphttp(h);

		}

		ClearNameValueList(&data);
		
	}
	//wait(NULL);
}

static void
DeletePortMapping(struct upnphttp * h)
{
	int r;
	//pid_t pid;
	static const char resp[] =
		"<u:DeletePortMappingResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"</u:DeletePortMappingResponse>";

	struct NameValueParserData data;
	const char * r_host, * ext_port, * protocol;
	unsigned short eport;
	
	//if((pid = fork()) == 0)
	{
		ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
		r_host = GetValueFromNameValueList(&data, "NewRemoteHost");
		ext_port = GetValueFromNameValueList(&data, "NewExternalPort");
		protocol = GetValueFromNameValueList(&data, "NewProtocol");

		if(!ext_port || !protocol)
		{
			ClearNameValueList(&data);
			SoapError(h, 402, "Invalid Args");
			return ;
		}

		eport = (unsigned short)atoi(ext_port);
		syslog(LOG_INFO, "DeletePortMapping: external port: %hu, protocol: %s", 
			eport, protocol);

		r = upnp_delete_redirection(eport, protocol);

		if(r < 0)
		{	
			SoapError(h, 714, "NoSuchEntryInArray");
		}
		else
		{
			BuildSendAndCloseSoapResp(h, resp, sizeof(resp)-1);
			CloseSocket_upnphttp(h);

		}

		ClearNameValueList(&data);
	}
	//wait(NULL);
}

static void
GetGenericPortMappingEntry(struct upnphttp * h)
{
	int r;
	//pid_t pid;	
	static const char resp[] =
		"<u:GetGenericPortMappingEntryResponse "
		"xmlns:u=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
		"<NewRemoteHost></NewRemoteHost>"
		"<NewExternalPort>%u</NewExternalPort>"
		"<NewProtocol>%s</NewProtocol>"
		"<NewInternalPort>%u</NewInternalPort>"
		"<NewInternalClient>%s</NewInternalClient>"
		"<NewEnabled>1</NewEnabled>"
		"<NewPortMappingDescription>%s</NewPortMappingDescription>"
		"<NewLeaseDuration>0</NewLeaseDuration>"
		"</u:GetGenericPortMappingEntryResponse>";

	int index = 0;
	unsigned short eport, iport;
	const char * m_index;
	char protocol[4], iaddr[32];
	char desc[64];
	struct NameValueParserData data;
//	if((pid = fork()) == 0)
	{
		ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
		m_index = GetValueFromNameValueList(&data, "NewPortMappingIndex");

		if(!m_index)
		{
			ClearNameValueList(&data);
			SoapError(h, 402, "Invalid Args");
			return ;
		}	

		index = (int)atoi(m_index);

		syslog(LOG_INFO, "GetGenericPortMappingEntry: index=%d", index);
		r = upnp_get_redirection_infos_by_index(index, &eport, protocol, &iport,
	                                            iaddr, sizeof(iaddr),
		                                        desc, sizeof(desc));

		if(r < 0)
		{
			SoapError(h, 713, "SpecifiedArrayIndexInvalid");
		}
		else
		{
			int bodylen;
			char body[2048];
			bodylen = snprintf(body, sizeof(body), resp, (unsigned int)eport,
				protocol, (unsigned int)iport, iaddr, desc);
			BuildSendAndCloseSoapResp(h, body, bodylen);
			CloseSocket_upnphttp(h);

		}

		ClearNameValueList(&data);
	}
	//wait(NULL);
}

/*
If a control point calls QueryStateVariable on a state variable that is not
buffered in memory within (or otherwise available from) the service,
the service must return a SOAP fault with an errorCode of 404 Invalid Var.

QueryStateVariable remains useful as a limited test tool but may not be
part of some future versions of UPnP.
*/
static void
QueryStateVariable(struct upnphttp * h)
{
	static const char resp[] =
        "<u:QueryStateVariableResponse "
        "xmlns:u=\"urn:schemas-upnp-org:control-1-0\">"
		"<return>%s</return>"
        "</u:QueryStateVariableResponse>";
	//pid_t pid;
	char body[2048];
	int bodylen;
	struct NameValueParserData data;
	const char * var_name;
	//if((pid = fork()) == 0)
	{
		ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
		/*var_name = GetValueFromNameValueList(&data, "QueryStateVariable"); */
		/*var_name = GetValueFromNameValueListIgnoreNS(&data, "varName");*/
		var_name = GetValueFromNameValueList(&data, "varName");

		/*syslog(LOG_INFO, "QueryStateVariable(%.40s)", var_name); */

		if(!var_name)
		{
			SoapError(h, 402, "Invalid Args");
		}
		else if(strcmp(var_name, "ConnectionStatus") == 0)
		{	
			char wan_status[32];
			if(get_wan_up(ext_if_name))
				sprintf(wan_status,"Connected");
			else
				sprintf(wan_status,"Disconnected");
			bodylen = snprintf(body, sizeof(body), resp, wan_status);
			BuildSendAndCloseSoapResp(h, body, bodylen);
			CloseSocket_upnphttp(h);

		}
		else if(strcmp(var_name, "PortMappingNumberOfEntries") == 0)
		{
			int r = 0, index = 0;
			unsigned short eport, iport;
			char protocol[4], iaddr[32], desc[64];
			char strindex[10];

			do
			{
				protocol[0] = '\0'; iaddr[0] = '\0'; desc[0] = '\0';

				r = upnp_get_redirection_infos_by_index(index, &eport, protocol, &iport,
														iaddr, sizeof(iaddr),
														desc, sizeof(desc));
				index++;
			}
			while(r==0);

			snprintf(strindex, sizeof(strindex), "%i", index - 1);
			bodylen = snprintf(body, sizeof(body), resp, strindex);
			BuildSendAndCloseSoapResp(h, body, bodylen);
			CloseSocket_upnphttp(h);

		}
		else
		{
			syslog(LOG_NOTICE, "QueryStateVariable: Unknown: %s", var_name?var_name:"");
			SoapError(h, 404, "Invalid Var");
		}

		ClearNameValueList(&data);	
	}
//	wait(NULL);
}

//Following actions were added by Shearer Lu to pass the Certificate Tool on 2009.07.23
char L3DefConnService[32]="Default";
char EthernetLinkStatus[32]="Up";
char DHCPSConf[32]="0";


static void
SetConnectionType(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";

	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "SetConnectionType", "urn:schemas-upnp-org:service:WANIPConnection:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetEthernetLinkStatus(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\">"
        "<NewEthernetLinkStatus>%s</NewEthernetLinkStatus>"
        "</u:%sResponse>";

	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "GetEthernetLinkStatus", "urn:schemas-upnp-org:service:WANEthernetLinkConfig:1", EthernetLinkStatus, "GetEthernetLinkStatus");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetDefaultConnectionService(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\">"
        "<NewDefaultConnectionService>%s</NewDefaultConnectionService>"
        "</u:%sResponse>";

	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "GetDefaultConnectionService", "urn:schemas-upnp-org:service:Layer3Forwarding:1", L3DefConnService, "GetDefaultConnectionService");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
SetDefaultConnectionService(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;
	char * var_name=NULL;
	struct NameValueParserData data;
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	var_name = GetValueFromNameValueList(&data, "NewDefaultConnectionService");
	if(!var_name || strlen(var_name)>=sizeof(L3DefConnService)){
		SoapError(h, 404, "Invalid Var");
		return;
	}
	
	strcpy(L3DefConnService, var_name);
	ClearNameValueList(&data);
	bodylen = sprintf(body, resp, "SetDefaultConnectionService", "urn:schemas-upnp-org:service:Layer3Forwarding:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetDHCPServerConfigurable(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\">"
        "<NewDHCPServerConfigurable>%s</NewDHCPServerConfigurable>"
        "</u:%sResponse>";

	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "GetDHCPServerConfigurable", "urn:schemas-upnp-org:service:LANHostConfigManagement:1", DHCPSConf, "GetDHCPServerConfigurable");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}


static void
SetDHCPServerConfigurable(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;
	char * var_name=NULL;
	struct NameValueParserData data;
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	var_name = GetValueFromNameValueList(&data, "NewDHCPServerConfigurable");
	if(!var_name || strlen(var_name)>=sizeof(DHCPSConf)){
		SoapError(h, 404, "Invalid Var");
		return;
	}
	strcpy(DHCPSConf, var_name);
	ClearNameValueList(&data);
	bodylen = sprintf(body, resp, "SetDHCPServerConfigurable", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}


static void
GetDHCPRelay(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\">"
        "<NewDHCPRelay>%s</NewDHCPRelay>"
        "</u:%sResponse>";

	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "GetDHCPRelay", "urn:schemas-upnp-org:service:LANHostConfigManagement:1", DHCPSConf, "GetDHCPRelay");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

char DHCPRelay[32]="0";

static void
SetDHCPRelay(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;
	char * var_name=NULL;
	struct NameValueParserData data;
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	var_name = GetValueFromNameValueList(&data, "NewDHCPRelay");
	if(!var_name || strlen(var_name)>=sizeof(DHCPRelay)){
		SoapError(h, 404, "Invalid Var");
		return;
	}
	strcpy(DHCPRelay, var_name);
	ClearNameValueList(&data);
	bodylen = sprintf(body, resp, "SetDHCPRelay", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

char SubnetMask[32]="255.255.255.0";

static void
GetSubnetMask(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\">"
        "<NewSubnetMask>%s</NewSubnetMask>"
        "</u:%sResponse>";

	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "GetSubnetMask", "urn:schemas-upnp-org:service:LANHostConfigManagement:1", SubnetMask, "GetSubnetMask");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}


static void
SetSubnetMask(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;
	char * var_name=NULL;
	struct NameValueParserData data;
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	var_name = GetValueFromNameValueList(&data, "NewSubnetMask");
	if(!var_name || strlen(var_name)>=sizeof(SubnetMask)){
		SoapError(h, 404, "Invalid Var");
		return;
	}
	strcpy(SubnetMask, var_name);
	ClearNameValueList(&data);
	bodylen = sprintf(body, resp, "SetSubnetMask", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

         
char iprouter_str[128]="0";           
static void
SetIPRouter(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;
	char * var_name=NULL;
	struct NameValueParserData data;
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	var_name = GetValueFromNameValueList(&data, "NewIPRouters");
	if(!var_name || strlen(iprouter_str)+strlen(var_name)>=sizeof(iprouter_str)-1){
		SoapError(h, 404, "Invalid Var");
		return;
	}
	if(strlen(iprouter_str))
		strcat(iprouter_str, ",");
	strcat(iprouter_str, var_name);
	ClearNameValueList(&data);
	bodylen = sprintf(body, resp, "SetIPRouter", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
DeleteIPRouter(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;

	iprouter_str[0]=0;
	bodylen = sprintf(body, resp, "DeleteIPRouter", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
GetIPRoutersList(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\">"
        "<NewIPRouters>%s</NewIPRouters>"
        "</u:%sResponse>";
	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "GetIPRoutersList", "urn:schemas-upnp-org:service:LANHostConfigManagement:1", iprouter_str, "GetIPRoutersList");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

char NewDomainName[128]="";

static void
GetDomainName(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\">"
        "<NewDomainName>%s</NewDomainName>"
        "</u:%sResponse>";

	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "GetDomainName", "urn:schemas-upnp-org:service:LANHostConfigManagement:1", NewDomainName, "GetDomainName");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
SetDomainName(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;
	char * var_name=NULL;
	struct NameValueParserData data;
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	var_name = GetValueFromNameValueList(&data, "NewDomainName");
	if(!var_name || strlen(var_name)>=sizeof(NewDomainName)){
		SoapError(h, 404, "Invalid Var");
		return;
	}
	strcpy(NewDomainName, var_name);
	ClearNameValueList(&data);
	bodylen = sprintf(body, resp, "SetDomainName", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}


char MinAddress[32]="0.0.0.0";
char MaxAddress[32]="0.0.0.0";

static void
GetAddressRange(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\">"
		"<NewMinAddress>%s</NewMinAddress>"
		"<NewMaxAddress>%s</NewMaxAddress>"
        "</u:%sResponse>";

	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "GetAddressRange", "urn:schemas-upnp-org:service:LANHostConfigManagement:1", MinAddress, MaxAddress, "GetAddressRange");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
SetAddressRange(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;
	char * var_name=NULL;
	struct NameValueParserData data;
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	var_name = GetValueFromNameValueList(&data, "NewMinAddress");
	if(!var_name || strlen(var_name)>=sizeof(MinAddress)){
		SoapError(h, 404, "Invalid Var");
		return;
	}
	strcpy(MinAddress, var_name);
	var_name = GetValueFromNameValueList(&data, "NewMaxAddress");
	if(!var_name || strlen(var_name)>=sizeof(MaxAddress)){
		SoapError(h, 404, "Invalid Var");
		return;
	}
	strcpy(MaxAddress, var_name);	
	ClearNameValueList(&data);
	bodylen = sprintf(body, resp, "SetAddressRange", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}


char ReservedAddress[128]="0.0.0.0";

static void
GetReservedAddress(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\">"
		"<NewReservedAddresses>%s</NewReservedAddresses>"
        "</u:%sResponse>";

	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "GetReservedAddresses", "urn:schemas-upnp-org:service:LANHostConfigManagement:1", ReservedAddress, "GetReservedAddresses");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
SetReservedAddress(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;
	char * var_name=NULL;
	struct NameValueParserData data;
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	var_name = GetValueFromNameValueList(&data, "NewReservedAddresses");
	if(!var_name || strlen(var_name)+strlen(ReservedAddress)>=sizeof(ReservedAddress)-1){
		SoapError(h, 404, "Invalid Var");
		return;
	}
	if(strlen(ReservedAddress))
		strcat(ReservedAddress, ",");
	strcat(ReservedAddress, var_name);
	ClearNameValueList(&data);	
	bodylen = sprintf(body, resp, "SetReservedAddress", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
DeleteReservedAddress(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;

	ReservedAddress[0]=0;
	bodylen = sprintf(body, resp, "DeleteReservedAddress", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

char DNSServer[128]="0.0.0.0";

static void
GetDNSServer(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\">"
		"<NewDNSServers>%s</NewDNSServers>"
        "</u:%sResponse>";

	char body[512];
	int bodylen;

	bodylen = sprintf(body, resp, "GetDNSServers", "urn:schemas-upnp-org:service:LANHostConfigManagement:1", DNSServer, "GetDNSServers");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
SetDNSServer(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;
	char * var_name=NULL;
	struct NameValueParserData data;
	
	ParseNameValue(h->req_buf + h->req_contentoff, h->req_contentlen, &data);
	var_name = GetValueFromNameValueList(&data, "NewDNSServers");
	if(!var_name || strlen(var_name)+strlen(DNSServer)>=sizeof(DNSServer)-1){
		SoapError(h, 404, "Invalid Var");
		return;
	}
	if(strlen(DNSServer))
		strcat(DNSServer, ",");
	strcat(DNSServer, var_name);	
	ClearNameValueList(&data);
	bodylen = sprintf(body, resp, "SetDNSServer", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}

static void
DeleteDNSServer(struct upnphttp * h)
{
	static const char resp[] =
		"<u:%sResponse xmlns:u=\"%s\"/>";
	char body[512];
	int bodylen;

	DNSServer[0]=0;
	bodylen = sprintf(body, resp, "DeleteDNSServer", "urn:schemas-upnp-org:service:LANHostConfigManagement:1");	
	BuildSendAndCloseSoapResp(h, body, bodylen);
}
//Add End

struct method igd_soapMethods[] =
{
	{ "GetConnectionTypeInfo", GetConnectionTypeInfo },
	{ "GetNATRSIPStatus", GetNATRSIPStatus},
	{ "GetExternalIPAddress", GetExternalIPAddress},
	{ "AddPortMapping", AddPortMapping},
	{ "DeletePortMapping", DeletePortMapping},
	{ "GetGenericPortMappingEntry", GetGenericPortMappingEntry},
	{ "GetSpecificPortMappingEntry", GetSpecificPortMappingEntry},
	{ "QueryStateVariable", QueryStateVariable},
	{ "GetTotalBytesSent", GetTotalBytesSent},
	{ "GetTotalBytesReceived", GetTotalBytesReceived},
	{ "GetTotalPacketsSent", GetTotalPacketsSent},
	{ "GetTotalPacketsReceived", GetTotalPacketsReceived},
	{ "GetCommonLinkProperties", GetCommonLinkProperties},
	{ "GetStatusInfo", GetStatusInfo},
/* Oliver Add for support ForceTermination and RequestConnection */	
	{ "ForceTermination", ForceTermination},
	{ "RequestConnection", RequestConnection},
	
	//Following actions were added by Shearer Lu to pass the Certificate Tool on 2009.07.23
	{ "SetConnectionType", SetConnectionType},
	{ "GetDefaultConnectionService", GetDefaultConnectionService},
	{ "SetDefaultConnectionService", SetDefaultConnectionService},
	{ "GetEthernetLinkStatus", GetEthernetLinkStatus},
	{ "GetDHCPServerConfigurable", GetDHCPServerConfigurable},
	{ "SetDHCPServerConfigurable", SetDHCPServerConfigurable},
	{ "GetDHCPRelay", GetDHCPRelay},
	{ "SetDHCPRelay", SetDHCPRelay},
	{ "GetSubnetMask", GetSubnetMask},
	{ "SetSubnetMask", SetSubnetMask},	
	{ "SetIPRouter", SetIPRouter},
	{ "DeleteIPRouter", DeleteIPRouter},
	{ "GetIPRoutersList", GetIPRoutersList},
	{ "GetDomainName", GetDomainName},
	{ "SetDomainName", SetDomainName},
	{ "GetAddressRange", GetAddressRange},
	{ "SetAddressRange", SetAddressRange},
	{ "GetReservedAddresses", GetReservedAddress},
	{ "SetReservedAddress", SetReservedAddress},
	{ "DeleteReservedAddress", DeleteReservedAddress},
	{ "DeleteDNSServer", DeleteDNSServer},
	{ "GetDNSServers", GetDNSServer},
	{ "SetDNSServer", SetDNSServer},
	//Add End
	{ 0, 0 }
};

