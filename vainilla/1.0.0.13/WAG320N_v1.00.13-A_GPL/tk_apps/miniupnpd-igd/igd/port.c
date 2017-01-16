/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file contain some function which you maybe need change
 *      when you portting.
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
#include <wait.h>
/* for BSD's sysctl */
#include <sys/param.h>
#include <sys/sysctl.h>

/* unix sockets */
#include "maco.h"
#include "igd_globalvars.h"
#include "upnphttp.h"
#include "upnpevent.h"
#include "upnphttp_func.h"
#include "minissdp.h"
#include "upnpdescgen.h"
#include "getifaddr.h"
#include "upnpsoap.h"
#include "igd_redirect.h"
#include "port.h"

char *getWanIPAddress(void);

int get_wan_up(char *ifname)
{
	if(access("/tmp/wan_uptime", F_OK) == 0)
		return 1;
	else
		return 0;
}
#define	DSL_STATUS_FILE		"/tmp/dsl_status"
int start_wan(void)
{
	char buf[128]={0};
	
	if(access("/tmp/wan_uptime", F_OK) != 0){
		system("rc wan start");
		sprintf(buf, "/usr/sbin/adslctl info > %s 2>/dev/null", DSL_STATUS_FILE);
		system(buf);
	}		
	return 0;
}

int  stop_wan(void)
{
	system("rc wan stop");
	return 0;
}

extern time_t startup_time;
unsigned int get_uptime(void)
{
	FILE *fp;
	unsigned int uptime;
	unsigned int uptime2;

	uptime2 = time(NULL) - startup_time;
	fp = fopen("/proc/uptime","r");
	
	if( fp == NULL )
		return uptime2;
	
	fscanf(fp,"%d",&uptime);
	fclose(fp);
	if(uptime2 > uptime)
		return uptime;
	return uptime2;
}

int check_wancfg()
{
	return 0;
}

int check_wanip()
{
	static int wan_status = 0;/*default set to down*/
	static char wan_ip[sizeof("255.255.255.255")]="";
	char *p;
	int ipchanged = 0;
	int now_status = 0;

	if(access("/tmp/wan_uptime", F_OK) != 0)
		now_status = 1;

	if( now_status == 0 )
	{
		  if( (p = getWanIPAddress()) != NULL)
		   {
			if(strcmp(p, wan_ip) != 0)
			{
				ipchanged = 1;
				snprintf(wan_ip, sizeof(wan_ip), "%s", p);
			}
			strcpy(wan_ip, p);
			free(p);	
		   }
	}
	else
	{
		bzero(wan_ip, sizeof(wan_ip));
	}

	
	if(wan_status == now_status && ipchanged == 0)
	{
		wan_status = now_status;
		return 0;
	}
	else
	{
		wan_status = now_status;
		return 1;
	}
}

int check_wanppp()
{
	static int wan_status = 0;/*default set to down*/
	static char wan_ip[sizeof("255.255.255.255")]="";
	char *p;
	int ipchanged = 0;
	int now_status = 0;

	if(access("/tmp/wan_uptime", F_OK) != 0)
		now_status = 1;

	if( now_status == 0 )
	{
		  if( (p = getWanIPAddress()) != NULL)
		   {
			if(strcmp(p, wan_ip) != 0)
			{
				ipchanged = 1;
				snprintf(wan_ip, sizeof(wan_ip), "%s", p);
			}
			strcpy(wan_ip, p);
			free(p);	
		   }
	}
	else
	{
		bzero(wan_ip, sizeof(wan_ip));
	}
	
	if(wan_status == now_status && ipchanged == 0)
	{
		wan_status = now_status;
		return 0;
	}
	else
	{
		wan_status = now_status;
		return 1;
	}
}

int check_layer3()
{
	return 0;
}

int check_wanEthCfg()
{
	return 0;
}

int check_lanHcfg()
{
	return 0;
}

int get_EthernetLinkStatus(char *buf)
{
	return sprintf(buf,"%s","Up");
}

int get_connectType(char *buf)
{
	return sprintf(buf,"%s","IP_Routed");
}
 
int get_connectStatus(char *buf)
{
	if(get_wan_up(ext_if_name))
		sprintf(buf,"%s","Connected");
	else
		sprintf(buf,"%s","Disconnected");
	return strlen(buf);
}

int get_XName(char *buf)
{
	return sprintf(buf,"%s","Local Area Connection");
}

int get_Exip(char *buf)
{
	char ip[20];

	if(use_ext_ip_addr[0])
		sprintf(ip,"%s",use_ext_ip_addr);
	else if(getifaddr(ext_if_name, ip, INET_ADDRSTRLEN) < 0)
	{
		syslog(LOG_ERR, "Failed to get ip address for interface %s",
			ext_if_name);
		strncpy(ip, "0.0.0.0", INET_ADDRSTRLEN);
	}
	return sprintf(buf,"%s",ip);
}

int get_MappNum(char *buf)
{
	int r = 0, index = 0;
	unsigned short eport, iport;
	char protocol[4], iaddr[32], desc[64];
//	int fd[2];
//	char tmp[10];
//	pid_t pid;

//	pipe(fd);

//	if((pid = fork()) == 0)
	{
//		char buf[10];
		do
		{
			protocol[0] = '\0'; iaddr[0] = '\0'; desc[0] = '\0';

			r = upnp_get_redirection_infos_by_index(index, &eport, protocol, &iport,
												iaddr, sizeof(iaddr),
												desc, sizeof(desc));
			index++;
		}while(r==0);
//		sprintf(buf,"%d",index);
//		write(fd[1],buf,10);
//		exit(0);
	}
//	wait(NULL);
//	read(fd[0],tmp,10);
//	close(fd[0]);
//	close(fd[1]);
//	sscanf(tmp,"%d",&index);

	return sprintf(buf, "%i", index - 1);
}

int get_conserver(char *buf)
{
	if(wan_type == 0)
		sprintf(buf,"WANIPConnection");
	else
		sprintf(buf,"WANPPPConnection");
	return strlen(buf);
}

