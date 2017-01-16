/* $Id: minissdp.c,v 1.2 2009-07-24 05:35:45 shearer_lu Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>

#include "maco.h"
#include "upnphttp.h"
#include "minissdp.h"

#define TYPE_UUID				0
#define TYPE_ROOT				1
#define TYPE_DEVICE_SERVICE	2
#define TYPE_ALL					3

/* SSDP ip/port */
#define SSDP_PORT (1900)
#define SSDP_MCAST_ADDR ("239.255.255.250")
char uuidvalue[64] = "uuid:00000000-0000-0000-0000-000000000000";
struct  service_type_uuid *known_service_types;
char root_device_path[128];

static int
AddMulticastMembership(int s, const char * ifaddr)
{
	struct ip_mreq imr;	/* Ip multicast membership */

    /* setting up imr structure */
    imr.imr_multiaddr.s_addr = inet_addr(SSDP_MCAST_ADDR);
    /*imr.imr_interface.s_addr = htonl(INADDR_ANY);*/
    imr.imr_interface.s_addr = inet_addr(ifaddr);
	
	if (setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&imr, sizeof(struct ip_mreq)) < 0)
	{
        syslog(LOG_ERR, "setsockopt(udp, IP_ADD_MEMBERSHIP): %m");
		return -1;
    }

	return 0;
}

int
OpenAndConfSSDPReceiveSocket(const char * ifaddr,
                             int n_add_listen_addr,
							 const char * * add_listen_addr)
{
	int s;
	struct sockaddr_in sockname;
	int bReuseaddr=1;
	
	if( (s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		syslog(LOG_ERR, "socket(udp): %m");
		return -1;
	}	
	
	memset(&sockname, 0, sizeof(struct sockaddr_in));
    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(SSDP_PORT);
	/* NOTE : it seems it doesnt work when binding on the specific address */
    /*sockname.sin_addr.s_addr = inet_addr(UPNP_MCAST_ADDR);*/
    sockname.sin_addr.s_addr = htonl(INADDR_ANY);
    /*sockname.sin_addr.s_addr = inet_addr(ifaddr);*/
	/*We must set it reuseaddr mode. Oliver.Hao.07.8.6*/
	setsockopt(s,SOL_SOCKET,SO_REUSEADDR,&bReuseaddr,sizeof(int));

    if(bind(s, (struct sockaddr *)&sockname, sizeof(struct sockaddr_in)) < 0)
	{
		syslog(LOG_ERR, "bind(udp): %m");
		close(s);
		return -1;
    }

	if(AddMulticastMembership(s, ifaddr) < 0)
	{
		close(s);
		return -1;
	}
	while(n_add_listen_addr>0)
	{
		n_add_listen_addr--;
		if(AddMulticastMembership(s, add_listen_addr[n_add_listen_addr]) < 0)
		{
			syslog(LOG_WARNING, "Failed to add membership for address %s. EXITING", 
			       add_listen_addr[n_add_listen_addr] );
		}
	}

	return s;
}

/* open the UDP socket used to send SSDP notifications to
 * the multicast group reserved for them */
int
OpenAndConfSSDPNotifySocket(const char * addr)
{
	int s;
	unsigned char loopchar = 0;
	int bcast = 1;
	struct in_addr mc_if;
	struct sockaddr_in sockname;
	
	if( (s = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
	{
		syslog(LOG_ERR, "socket(udp_notify): %m");
		return -1;
	}

	mc_if.s_addr = inet_addr(addr);

	if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopchar, sizeof(loopchar)) < 0)
	{
		syslog(LOG_ERR, "setsockopt(udp_notify, IP_MULTICAST_LOOP): %m");
		close(s);
		return -1;
	}

	if(setsockopt(s, IPPROTO_IP, IP_MULTICAST_IF, (char *)&mc_if, sizeof(mc_if)) < 0)
	{
		syslog(LOG_ERR, "setsockopt(udp_notify, IP_MULTICAST_IF): %m");
		close(s);
		return -1;
	}
	
	if(setsockopt(s, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast)) < 0)
	{
		syslog(LOG_ERR, "setsockopt(udp_notify, SO_BROADCAST): %m");
		close(s);
		return -1;
	}

	memset(&sockname, 0, sizeof(struct sockaddr_in));
    sockname.sin_family = AF_INET;
    sockname.sin_addr.s_addr = inet_addr(addr);

    if (bind(s, (struct sockaddr *)&sockname, sizeof(struct sockaddr_in)) < 0)
	{
		syslog(LOG_ERR, "bind(udp_notify): %m");
		close(s);
		return -1;
    }

	return s;
}


void send_reply_search_name(int s, struct sockaddr_in sockname,
                  struct service_type_uuid *device_uuid,int type,
                  const char * host, unsigned short port)
{
	char buf[512];
	int l, n;
	
	l = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
			"Cache-Control: max-age=1800\r\n"
			"ST: %s\r\n"
			"USN: uuid:%s::%s\r\n"
			"EXT:\r\n"
			"SERVER: " MINIUPNPD_SERVER_STRING "\r\n"
			"Location: http://%s:%u" "%s" "\r\n"
			"\r\n",
			device_uuid->service_name,
			device_uuid->uuid,
			device_uuid->service_name,
			host, (unsigned int)port,
			root_device_path);
	n = sendto(s, buf, l, 0, (struct sockaddr *)&sockname, sizeof(struct sockaddr_in));
	if(n < 0)
	{
		syslog(LOG_ERR, "sendto(udp): %m");
	}
}

void send_reply_search_uuid(int s, struct sockaddr_in sockname,
                  struct service_type_uuid *device_uuid,int type,
                  const char * host, unsigned short port)
{
	char buf[512];
	int l, n;
	
	l = snprintf(buf, sizeof(buf), "HTTP/1.1 200 OK\r\n"
		"Cache-Control: max-age=1800\r\n"
		"ST: uuid:%s\r\n"
		"USN: uuid:%s\r\n"
		"EXT:\r\n"
		"SERVER: " MINIUPNPD_SERVER_STRING "\r\n"
		"Location: http://%s:%u" "%s" "\r\n"
		"\r\n",
		device_uuid->uuid,
		device_uuid->uuid,
		host, (unsigned int)port,
		root_device_path);
	n = sendto(s, buf, l, 0,(struct sockaddr *)&sockname, sizeof(struct sockaddr_in));
	if(n < 0)
	{
		syslog(LOG_ERR, "sendto(udp): %m");
	}
}


/*
 * response from a LiveBox (Wanadoo)
HTTP/1.1 200 OK
CACHE-CONTROL: max-age=1800
DATE: Thu, 01 Jan 1970 04:03:23 GMT
EXT:
LOCATION: http://192.168.0.1:49152/gatedesc.xml
SERVER: Linux/2.4.17, UPnP/1.0, Intel SDK for UPnP devices /1.2
ST: upnp:rootdevice
USN: uuid:75802409-bccb-40e7-8e6c-fa095ecce13e::upnp:rootdevice

 * response from a Linksys 802.11b :
HTTP/1.1 200 OK
Cache-Control:max-age=120
Location:http://192.168.5.1:5678/rootDesc.xml
Server:NT/5.0 UPnP/1.0
ST:upnp:rootdevice
USN:uuid:upnp-InternetGatewayDevice-1_0-0090a2777777::upnp:rootdevice
EXT:
 */

/* not really an SSDP "announce" as it is the response
 * to a SSDP "M-SEARCH" */
static void
SendSSDPAnnounce2(int s, struct sockaddr_in sockname,
                  struct service_type_uuid *device_uuid,int type,
                  const char * host, unsigned short port)
{
	/* TODO :
	 * follow guideline from document "UPnP Device Architecture 1.0"
	 * put in uppercase.
	 * DATE: is recommended
	 * SERVER: OS/ver UPnP/1.0 miniupnpd/1.0
	 * - check what to put in the 'Cache-Control' header 
	 * */
	 switch (type)
	 {
	 	case TYPE_ALL:
			if(device_uuid->service_type == ROOT_DEVICE)
			{
				if(strcmp(device_uuid->service_name,"upnp:rootdevice") == 0)
					send_reply_search_name(s,sockname, device_uuid,type,host, port);
				else
				{
					send_reply_search_name(s,sockname, device_uuid,type,host, port);
					send_reply_search_uuid(s,sockname, device_uuid,type,host, port);		
				}
			}

			if(device_uuid->service_type == EMBED_DEVICE)
			{
					send_reply_search_name(s,sockname, device_uuid,type,host, port);
					send_reply_search_uuid(s,sockname, device_uuid,type,host, port);		
			}

			if(device_uuid->service_type == SERVICE)
				send_reply_search_name(s,sockname, device_uuid,type,host, port);
			break;
		case TYPE_ROOT:
			send_reply_search_name(s,sockname, device_uuid,type,host, port);
			break;
		case TYPE_DEVICE_SERVICE:
			send_reply_search_name(s,sockname, device_uuid,type,host, port);
			break;
		case TYPE_UUID:
			send_reply_search_uuid(s,sockname, device_uuid,type,host, port);
			break;
		default :
			break;
	 }
}

void send_notify_uuid(struct sockaddr_in *p_sockname,int s, const char * host, unsigned short port,struct  service_type_uuid *service)
{
	char bufr[512];

	snprintf(bufr, sizeof(bufr), 
			"NOTIFY * HTTP/1.1\r\n"
			"HOST: %s:%d\r\n"
			"Cache-Control: max-age=1800\r\n"
				"Location: http://%s:%d" "%s""\r\n"
				"NT: uuid:%s\r\n"
				"NTS: ssdp:alive\r\n"
				/*"Server:miniupnpd/1.0 UPnP/1.0\r\n"*/
				"SERVER: " MINIUPNPD_SERVER_STRING "\r\n"
				"USN: uuid:%s\r\n"
				"\r\n",
				SSDP_MCAST_ADDR, SSDP_PORT,
				host, port,
				root_device_path,
			service->uuid,
			service->uuid);
	sendto(s, bufr, strlen(bufr), 0,(struct sockaddr *)p_sockname, sizeof(struct sockaddr_in) );
}

void send_notify_name(struct sockaddr_in *p_sockname,int s, const char * host, unsigned short port,struct  service_type_uuid *service)
{
	char bufr[512];

				snprintf(bufr, sizeof(bufr), 
				"NOTIFY * HTTP/1.1\r\n"
				"HOST: %s:%d\r\n"
				"Cache-Control: max-age=1800\r\n"
				"Location: http://%s:%d" "%s""\r\n"
				"NT: %s\r\n"
				"NTS: ssdp:alive\r\n"
				/*"Server:miniupnpd/1.0 UPnP/1.0\r\n"*/
				"SERVER: " MINIUPNPD_SERVER_STRING "\r\n"
				"USN: uuid:%s::%s\r\n"
				"\r\n",
				SSDP_MCAST_ADDR, SSDP_PORT,
				host, port,
				root_device_path,
			service->service_name,
			service->uuid, 
			service->service_name);
		sendto(s, bufr, strlen(bufr), 0,(struct sockaddr *)p_sockname, sizeof(struct sockaddr_in) );
}

void
SendSSDPNotifies(int s, const char * host, unsigned short port)
{
	struct sockaddr_in sockname;
	int i=0;

	memset(&sockname, 0, sizeof(struct sockaddr_in));
	sockname.sin_family = AF_INET;
	sockname.sin_port = htons(SSDP_PORT);
	sockname.sin_addr.s_addr = inet_addr(SSDP_MCAST_ADDR);
//				for(j = 0; j < 2; j++)
	while(known_service_types[i].service_name != NULL)
	{
		switch (known_service_types[i].service_type)
		{
			case ROOT_DEVICE:
				if(strcmp(known_service_types[i].service_name,"upnp:rootdevice") == 0)
					send_notify_name(&sockname,s,host,port,&known_service_types[i]);
				else
				{
					send_notify_name(&sockname,s,host,port,&known_service_types[i]);
					send_notify_uuid(&sockname,s,host,port,&known_service_types[i]);
				}
				break;
			case EMBED_DEVICE:
				send_notify_name(&sockname,s,host,port,&known_service_types[i]);
				send_notify_uuid(&sockname,s,host,port,&known_service_types[i]);
				break;
			case SERVICE:
				send_notify_name(&sockname,s,host,port,&known_service_types[i]);
				break;
			default :
				break;				
		}
		i++;
	}
}

extern char *my_strstr(const char *, const char *, int );

int check_m_search_line(char *buf)
{
	char *tmp;

	if((tmp = strstr(buf,"M-SEARCH")) == NULL)
		return -1;
	tmp = tmp + strlen("M-SEARCH");
	if((tmp = strstr(tmp,"*")) == NULL)
		return -1;
	tmp++;
	if((tmp = strstr(tmp,"HTTP/1.1")) == NULL)
		return -1;
	return 0;
}

int check_man(char *buf)
{
	char *tmp;

	if((tmp = my_strstr(buf,"MAN",1)) == NULL)
		return -1;
	tmp += strlen("MAN");
	if((tmp = strstr(tmp,"\"ssdp:discover\"")) == NULL)
		return -1;
	return 0;
}

int check_mx(char *buf)
{
	char *tmp;
	int i,state = 0;
	
	if((tmp = my_strstr(buf,"MX",1)) == NULL)
		return -1;
	tmp += strlen("MX");
	if((tmp = strstr(tmp,":")) == NULL)
		return -1;
	tmp ++;
	for(i = 0; i < 10; i++)
	{
		if((tmp[i] == '\r') ||(tmp[i] == '\n'))
			break;
		if((state == 0) && (tmp[i] == ' '))
			continue;
		if((tmp[i] < '0') || (tmp[i] > '9'))
			return -1;
		state = 1;
	}
	if((i == 10) || (state == 0))
		return -1;
	return 0;
}

int check_st(char *buf)
{
	return 0;	
}

int check_m_search(char *buf)
{
	if(check_m_search_line(buf) < 0)
	{
#ifdef DEBUG
		printf("%s[%d] : check search fail\n",__FUNCTION__,__LINE__);
#endif
		return -1;
	}
	if(check_man(buf) < 0)
	{
#ifdef DEBUG
		printf("%s[%d] : check MAN fail\n",__FUNCTION__,__LINE__);
#endif
		return -1;
	}
	if(check_mx(buf) < 0)
	{
#ifdef DEBUG
		printf("%s[%d] : check MX fail\n",__FUNCTION__,__LINE__);
#endif
		return -1;
	}
	if(check_st(buf) < 0)
	{
#ifdef DEBUG
		printf("%s[%d] : check st fail\n",__FUNCTION__,__LINE__);
#endif
		return -1;
	}
	return 0;
}

/* ProcessSSDPRequest()
 * process SSDP M-SEARCH requests and responds to them */
void
ProcessSSDPRequest(int s, const char * host, unsigned short port)
{
	int n;
	char bufr[1500];
	socklen_t len_r;
	struct sockaddr_in sendername;
	int i, l,j;
	char * st = 0;
	int st_len = 0;
	len_r = sizeof(struct sockaddr_in);

	n = recvfrom(s, bufr, sizeof(bufr), 0,
	             (struct sockaddr *)&sendername, &len_r);
	if(n < 0)
	{
		syslog(LOG_ERR, "recvfrom(udp): %m");
		return;
	}

	if(memcmp(bufr, "NOTIFY", 6) == 0)
	{
		/* ignore NOTIFY packets. We could log the sender and device type */
		return;
	}
	else if(memcmp(bufr, "M-SEARCH", 8) == 0)
	{
		i = 0;

		if(check_m_search(bufr) < 0)
			return ;
		while(i < n)
		{
			while(bufr[i] != '\r' || bufr[i+1] != '\n')
				i++;
			i += 2;
			if(strncasecmp(bufr+i, "st:", 3) == 0)
			{
				st = bufr+i+3;
				st_len = 0;
				while(*st == ' ' || *st == '\t') st++;
				while(st[st_len]!='\r' && st[st_len]!='\n') st_len++;
				/*syslog(LOG_INFO, "ST: %.*s", st_len, st);*/
				/*j = 0;*/
				/*while(bufr[i+j]!='\r') j++;*/
				/*syslog(LOG_INFO, "%.*s", j, bufr+i);*/
			}
		}
		/*syslog(LOG_INFO, "SSDP M-SEARCH packet received from %s:%d",
	           inet_ntoa(sendername.sin_addr),
	           ntohs(sendername.sin_port) );*/
		if(st)
		{
			/* TODO : doesnt answer at once but wait for a random time */
#ifdef DEBUG
			syslog(LOG_INFO, "SSDP M-SEARCH from %s:%d ST: %.*s",
	        	   inet_ntoa(sendername.sin_addr),
	           	   ntohs(sendername.sin_port),
				   st_len, st);
			printf("%s[%d] : SSDP M-SEARCH from %s:%d ST: %.*s",
				__FUNCTION__,__LINE__,
	        	   inet_ntoa(sendername.sin_addr),
	           	   ntohs(sendername.sin_port),
				   st_len, st);
#endif
			/* Responds to request with a device as ST header, IPConnection_service_types */
			for(i = 0; known_service_types[i].service_name != NULL; i++)
			{
				l = (int)strlen(known_service_types[i].service_name);
				if(l<=st_len && (0 == memcmp(st, known_service_types[i].service_name, l)))
				{
					SendSSDPAnnounce2(s, sendername,&known_service_types[i],
					                  TYPE_DEVICE_SERVICE, host, port);
					break;
				}
			}
			/* Responds to request with ST: ssdp:all */
			/* strlen("ssdp:all") == 8 */
			if(st_len==8 && (0 == memcmp(st, "ssdp:all", 8)))
			{
				for(i=0; known_service_types[i].service_name != NULL; i++)
				{
					l = (int)strlen(known_service_types[i].service_name);
					SendSSDPAnnounce2(s, sendername,
					                  &known_service_types[i], TYPE_ALL,
					                  host, port);
				}
			}
			/* responds to request by UUID value */
			j = -1;
			while(known_service_types[++j].service_name != NULL)
			{
				char uuid[256];
				l = (int)strlen(known_service_types[j].uuid) + strlen("uuid:");
				sprintf(uuid,"uuid:%s",known_service_types[j].uuid);
				if(l==st_len && (0 == memcmp(st, uuid, l)))
				{
					SendSSDPAnnounce2(s, sendername, &known_service_types[j], TYPE_UUID, host, port);
					break;
				}
			}
		}
		else
		{
			syslog(LOG_INFO, "Invalid SSDP M-SEARCH from %s:%d",
	        	   inet_ntoa(sendername.sin_addr), ntohs(sendername.sin_port));
		}
	}
	else
	{
		syslog(LOG_NOTICE, "Unknown udp packet received from %s:%d",
		       inet_ntoa(sendername.sin_addr), ntohs(sendername.sin_port));
	}
}

void send_goodbye_name(struct sockaddr_in *p_sockname, int s, struct  service_type_uuid *service)
{
	int n;
	char bufr[512];

        snprintf(bufr, sizeof(bufr),
                 "NOTIFY * HTTP/1.1\r\n"
                 "HOST:%s:%d\r\n"
                 "NT:%s\r\n"
                 "USN:uuid:%s::%s\r\n"
                 "NTS:ssdp:byebye\r\n"
                 "\r\n",
                 SSDP_MCAST_ADDR, SSDP_PORT,
				 service->service_name,
                 		service->uuid ,
                 		service->service_name);

	 n = sendto(s, bufr, strlen(bufr), 0,
                   (struct sockaddr *)p_sockname, sizeof(struct sockaddr_in) );
	if(n < 0)
	{
		syslog(LOG_ERR, "sendto(udp_shutdown): %m");
		return;
	}
}

void send_goodbye_uuid(struct sockaddr_in *p_sockname, int s, struct  service_type_uuid *service)
{
	int n;
	char bufr[512];

        snprintf(bufr, sizeof(bufr),
                 "NOTIFY * HTTP/1.1\r\n"
                 "HOST:%s:%d\r\n"
                 "NT:uuid:%s\r\n"
                 "USN:uuid:%s\r\n"
                 "NTS:ssdp:byebye\r\n"
                 "\r\n",
                 SSDP_MCAST_ADDR, SSDP_PORT,
				 service->uuid,
                 		service->uuid );

	 n = sendto(s, bufr, strlen(bufr), 0,
                   (struct sockaddr *)p_sockname, sizeof(struct sockaddr_in) );
	if(n < 0)
	{
		syslog(LOG_ERR, "sendto(udp_shutdown): %m");
		return;
	}
	
}
/* This will broadcast ssdp:byebye notifications to inform 
 * the network that UPnP is going down. */
int
SendSSDPGoodbye(int s)
{
	int i,j;
	struct sockaddr_in sockname;

    memset(&sockname, 0, sizeof(struct sockaddr_in));
    sockname.sin_family = AF_INET;
    sockname.sin_port = htons(SSDP_PORT);
    sockname.sin_addr.s_addr = inet_addr(SSDP_MCAST_ADDR);

	for( j = 0; j < 2; j++)
	{
		i = -1;
		while(known_service_types[++i].service_name != NULL)
		{
			switch (known_service_types[i].service_type)
			{
				case ROOT_DEVICE:
					if(strcmp(known_service_types[i].service_name,"upnp:rootdevice") == 0)
						send_goodbye_name(&sockname,s,&known_service_types[i]);
					else
					{
						send_goodbye_name(&sockname,s,&known_service_types[i]);
						send_goodbye_uuid(&sockname,s,&known_service_types[i]);
					}
					break;
				case EMBED_DEVICE:
					send_goodbye_name(&sockname,s,&known_service_types[i]);
					send_goodbye_uuid(&sockname,s,&known_service_types[i]);
					break;
				case SERVICE:
					send_goodbye_name(&sockname,s,&known_service_types[i]);
					break;
				default :
					break;				
			} 
		}
	}

    return 0;
}

