/* $Id: miniupnpd.c,v 1.2 2009-07-24 05:35:45 shearer_lu Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

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
#include <sys/queue.h> 
/* for BSD's sysctl */
#include <sys/param.h>
#include <sys/sysctl.h>

#include "maco.h"
#include "upnphttp.h"
#include "upnpevent.h"
#include "upnphttp_func.h"
#include "upnpdescgen.h"
#include "getifaddr.h"
#include "daemonize.h"
#include "upnpsoap.h"
#include "minissdp.h"
#include "miniupnpd.h"
#include "igd_redirect.h"

static volatile int quitting = 0;

/* startup time */
time_t startup_time = 0;
/* OpenAndConfHTTPSocket() :
 * setup the socket used to handle incoming HTTP connections. */
static int
OpenAndConfHTTPSocket(const char * addr, unsigned short port)
{
	int s;
	int i = 1;
	struct sockaddr_in listenname;

	if( (s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		syslog(LOG_ERR, "socket(http): %m");
		return -1;
	}

	if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i)) < 0)
	{
		syslog(LOG_WARNING, "setsockopt(http, SO_REUSEADDR): %m");
	}

	memset(&listenname, 0, sizeof(struct sockaddr_in));
	listenname.sin_family = AF_INET;
	listenname.sin_port = htons(port);
	listenname.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(s, (struct sockaddr *)&listenname, sizeof(struct sockaddr_in)) < 0)
	{
		syslog(LOG_ERR, "bind(http): %m");
		close(s);
		return -1;
	}

	if(listen(s, 6) < 0)
	{
		syslog(LOG_ERR, "listen(http): %m");
		close(s);
		return -1;
	}

	return s;
}

/* Handler for the SIGTERM signal (kill) */
static void
sigterm(int sig)
{
	/*int save_errno = errno;*/
	signal(sig, SIG_IGN);

	syslog(LOG_NOTICE, "received signal %d, good-bye", sig);
	printf("[%s():%d] : Call stop function\n", __FUNCTION__, __LINE__);
	quitting = 1;
	/*errno = save_errno;*/
}

/* record the startup time, for returning uptime */
static void
set_startup_time(int sysuptime)
{
	startup_time = time(NULL);
	if(sysuptime)
	{
		/* use system uptime instead of daemon uptime */
#if defined(__linux__)
		char buff[64];
		int uptime, fd;
		fd = open("/proc/uptime", O_RDONLY);
		if(fd < 0)
		{
			syslog(LOG_ERR, "open(\"/proc/uptime\" : %m");
		}
		else
		{
			memset(buff, 0, sizeof(buff));
			read(fd, buff, sizeof(buff) - 1);
			uptime = atoi(buff);
//			syslog(LOG_INFO, "system uptime is %d seconds", uptime);
			close(fd);
			startup_time -= uptime;
		}
		close(fd);
#else
		struct timeval boottime;
		size_t size = sizeof(boottime);
		int name[2] = { CTL_KERN, KERN_BOOTTIME };
		if(sysctl(name, 2, &boottime, &size, NULL, 0) < 0)
		{
			syslog(LOG_ERR, "sysctl(\"kern.boottime\") failed");
		}
		else
		{
			startup_time = boottime.tv_sec;
		}
#endif
	}
}

/* init phase :
 * 1) read configuration file
 * 2) read command line arguments
 * 3) daemonize
 * 4) open syslog
 * 5) check and write pid file
 * 6) set startup time stamp
 * 7) compute presentation URL
 * 8) set signal handlers */
int
upnp_init(struct  service_type_uuid *service_type,char *pid_file,char *uuid,char *root_device,struct method *methods,struct http_desc *desc,int debug)
{
	pid_t pid;
	struct sigaction sa;

	known_service_types = service_type;
	strncpy(uuidvalue,uuid,64 -1);
	strncpy(root_device_path,root_device,128-1);
	soapMethods = methods;
	upnp_http_desc = desc;
	

	if(debug)
	{
		pid = getpid();
	}
	else
	{
		pid = daemonize();
	}

	if(checkforrunning(pid_file) < 0)
	{
		syslog(LOG_ERR, "MiniUPnPd is already running. EXITING");
		printf("%s[%d] : MiniUPnPd is already running. EXITING",__FUNCTION__,__LINE__);
		return 1;
	}	

	writepidfile(pid_file, pid);

	set_startup_time(1);

	/* set signal handler */
	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = sigterm;

	if (sigaction(SIGTERM, &sa, NULL))
	{
		syslog(LOG_ERR, "Failed to set SIGTERM handler. EXITING");
		return 1;
	}
	if (sigaction(SIGINT, &sa, NULL))
	{
		syslog(LOG_ERR, "Failed to set SIGTERM handler. EXITING");
		return 1;
	}
	
	return 0;
}

int miniupnp_deamon(struct runtime_vars *v, struct event_list *p_event_list)
{
	int i,j;
	int sudp, shttpl, snotify;
	LIST_HEAD(httplisthead, upnphttp) upnphttphead;
	struct eventlisthead event_handlehead;
	struct upnphttp * e = 0;
	struct event_handle * p_event = 0;
//	struct upnphttp * next;
//	struct event_handle * event_next;
	fd_set readset;	/* for select() */
	fd_set writeset;
	int max_fd;
	struct timeval timeout, timeofday, lasttimeofday = {0, 0};
	struct time_list * time_p;
	int ret=0;
	
	LIST_INIT(&upnphttphead);
	LIST_INIT(&event_handlehead);
	LIST_INIT(&time_head);

	/* open socket for SSDP connections */
	sudp = OpenAndConfSSDPReceiveSocket(v->listen_addr, v->n_add_listen_addr,
	                                    v->add_listen_addr);
	if(sudp < 0)
	{
		syslog(LOG_ERR, "Failed to open socket for receiving SSDP. EXITING");
		return 1;
	}
	/* open socket for HTTP connections */
	shttpl = OpenAndConfHTTPSocket(v->listen_addr, v->port);
	if(shttpl < 0)
	{
		syslog(LOG_ERR, "Failed to open socket for HTTP. EXITING");
		return 1;
	}
	//syslog(LOG_NOTICE, "listening on %s:%d", v->listen_addr, v->port);

	/* open socket for sending notifications */
	snotify = OpenAndConfSSDPNotifySocket(v->listen_addr);
	if(snotify < 0)
	{
		syslog(LOG_ERR, "Failed to open socket for sending SSDP notify "
		                "messages. EXITING");
		return 1;
	}

	/* main loop */
	while(!quitting)
	{
		/* Check if we need to send SSDP NOTIFY messages and do it if
		 * needed */
		if(gettimeofday(&timeofday, 0) < 0)
		{
			syslog(LOG_ERR, "gettimeofday(): %m");
			timeout.tv_sec = v->notify_interval;
			timeout.tv_usec = 0;
		}
		else
		{
			/* If this port mapping pair have time out, we should delete it */
			for(time_p = time_head.lh_first; time_p != NULL; time_p = time_p->entries.le_next)
			{
				if(time(NULL)  - time_p->add_time >=  time_p->timeout)
				{
					int r;
					r = upnp_delete_redirection(time_p->eport, time_p->protocol);
					if(r < 0)
					{	
#ifdef DEBUG
						printf("\n[ ERROR ]Delet Port %d Protocol:%s  Failed\n",time_p->eport, time_p->protocol);
#endif
					}
					else
					{
#ifdef DEBUG
						printf("\n[ TIMEOUT ]Delet Port %d Protocol:%s  successfully\n",time_p->eport, time_p->protocol);
#endif
					}
					LIST_REMOVE(time_p,entries);
				}
			}
			/* the comparaison is not very precise but who cares ? */
			if(timeofday.tv_sec >= (lasttimeofday.tv_sec + v->notify_interval))
			{
				/* Notifies time out, send out notify */
#ifdef DEBUG
				printf("%s[%d] : send ssdp notifies\n",__FUNCTION__,__LINE__);
#endif
				SendSSDPNotifies(snotify, v->listen_addr, (unsigned short)v->port);
				memcpy(&lasttimeofday, &timeofday, sizeof(struct timeval));
				timeout.tv_sec = v->notify_interval;
				timeout.tv_usec = 0;
			}
			else
			{
			      /* update notifies timeout */
				timeout.tv_sec = lasttimeofday.tv_sec + v->notify_interval
				                 - timeofday.tv_sec;
				if(timeofday.tv_usec > lasttimeofday.tv_usec)
				{
					timeout.tv_usec = 1000000 + lasttimeofday.tv_usec
					                  - timeofday.tv_usec;
					timeout.tv_sec--;
				}
				else
				{
					timeout.tv_usec = lasttimeofday.tv_usec - timeofday.tv_usec;
				}
			}

			/* Check time , this is  genereate by handle_subcribe*/
			j = 0;
			for(p_event = event_handlehead.lh_first; p_event != NULL; p_event = p_event->entries.le_next)
			{
				j++;
				if(p_event->state == 0)/* Just registed */
				{
					if(p_event->time_out > 0)
					{
						if((time(NULL) - p_event->creat_time_flag) > p_event->time_out)
						{
#ifdef DEBUG
							printf("%s[%d] : because create too long time, event need delete,evevt_URL=%s\n",__FUNCTION__,__LINE__,p_event->event_URL);
#endif
							LIST_REMOVE(p_event, entries);
							Delete_event(p_event);
						}
					}
				}

				if((p_event->state == 100) || (p_event->state == 200) )
				{
					if(time(NULL) - p_event->init_time_flag > 30)
					{
#ifdef DEBUG
						printf("%s[%d] : event still exist , we need abandon and init it\n",__FUNCTION__,__LINE__);
#endif
						p_event->state = 0;
						close(p_event->socket);
					}	
				}
			}			
#ifdef DEBUG
			//printf("%s[%d] : there is %d event handle\n",__FUNCTION__,__LINE__,j);
#endif
			/* find event obj , p_event_list is just EVENT_URL_LIST table */
			i = -1;
			while(p_event_list[++i].event_URL != NULL)
			{
				if(p_event_list[i].check_event() != 0)
				{
					/* Something change, we need send event */
#ifdef DEBUG
					printf("%s[%d] : event_URL<%s> is changed need send event\n",__FUNCTION__,__LINE__,p_event_list[i].event_URL);
#endif
					for(p_event = event_handlehead.lh_first; p_event != NULL; p_event = p_event->entries.le_next)
					{
						/* Find out who will be noticed */
						if(strcmp(p_event_list[i].event_URL, p_event->event_URL) == 0)
						{
#ifdef DEBUG
							printf("%s[%d] : client <ip=%s,url=%s> need to inform\n",__FUNCTION__,__LINE__,p_event->ip_addr,p_event->event_URL);
#endif
                                                 /* Create a socket, set p_event->state  100 when success, or -100 on error */                         
							preapre_send_event(p_event);
						}
					}
				}
			}
		}
		/* select open sockets (SSDP, HTTP listen, and all HTTP soap sockets) */
		FD_ZERO(&readset);
		FD_SET(sudp, &readset);
		FD_SET(shttpl, &readset);
		FD_ZERO(&writeset);
		
		if(sudp > shttpl)
			max_fd = sudp;
		else
			max_fd = shttpl;

		/* active HTTP connections count */
		i = 0;	
		j = 0;
		/* this is generate by socket shttpl */
		for(e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next)
		{
			j++;
			if((e->socket >= 0) && (e->state <= 2))
			{
				FD_SET(e->socket, &readset);
				if(e->socket > max_fd)
					max_fd = e->socket;
				i++;
			}
		}
#ifdef DEBUG
		//printf("%s[%d] : upnp http handle is %d\n",__FUNCTION__,__LINE__,j);
#endif
		/* we need to send out notify, find active event */
		for(p_event = event_handlehead.lh_first; p_event != NULL; p_event = p_event->entries.le_next)
		{
			if(p_event->socket >=0 && p_event->state == 100)
			{
			     /* if handle is init, add it to write FD, created by  preapre_send_event()*/
#ifdef DEBUG
				printf("%s[%d] : client<%s> is init need add it to writeSet\n",__FUNCTION__,__LINE__,p_event->ip_addr);
#endif
				FD_SET(p_event->socket, &writeset);
				if(p_event->socket > max_fd)
					max_fd = p_event->socket;
				i++;
			}

			if(p_event->socket >=0 && p_event->state == 200)
			{
#ifdef DEBUG
				printf("%s[%d] : client<%s> is connected need add it to readSet\n",__FUNCTION__,__LINE__,p_event->ip_addr);
#endif
				FD_SET(p_event->socket, &readset);
				if(p_event->socket > max_fd)
					max_fd = p_event->socket;
				i++;
			}
		}
		/* for debug */
#ifdef DEBUG
		if(i > 1)
		{
			syslog(LOG_DEBUG, "%d active incoming HTTP connections", i);
		}
#endif
		/* If timeout >= 5s, then it will recaculate in next circle, if timeout < 5s, we timeout is 0, we should send out notify */
		if(timeout.tv_sec > 5)
			timeout.tv_sec = 5;
#ifdef DEBUG
		printf("%s[%d] : timeout is %d\n",__FUNCTION__,__LINE__,(int)timeout.tv_sec);
#endif
		if(select(max_fd+1, &readset, &writeset, 0, &timeout) < 0)
		{
			if(quitting)
				goto shutdown;
			syslog(LOG_ERR, "select(all): %m");
			syslog(LOG_ERR, "Failed to select open sockets. ");
			continue ;	/* very serious cause of error */
		}
		/* process SSDP packets */
		if(FD_ISSET(sudp, &readset))
		{
			/*syslog(LOG_INFO, "Received UDP Packet");*/
			ProcessSSDPRequest(sudp, v->listen_addr, (unsigned short)v->port);
		}
		/* process active HTTP connections */
		/* LIST_FOREACH is not available under linux */
		for(e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next)
		{
			if(  (e->socket >= 0) && (e->state <= 2)
				&&(FD_ISSET(e->socket, &readset)) )
			{
				Process_upnphttp(e,&event_handlehead,p_event_list);
			}
		}
		
		for(p_event = event_handlehead.lh_first; p_event != NULL; p_event = p_event->entries.le_next){
			if( (p_event->state == 100)&&(FD_ISSET(p_event->socket, &writeset) )) {
#ifndef DEBUG
				printf("%s[%d] : miniupnpd send notify packet to <%s >\n",__FUNCTION__,__LINE__,p_event->ip_addr);
#endif
				send_event_notify(p_event,p_event_list);
				p_event->state = 200;/* send success, we will read message */
				p_event-> send_time_flag = time(NULL);
				ret=0;
			}

			if(  (p_event->state == 200)&&(FD_ISSET(p_event->socket, &readset))) {
#ifdef DEBUG
				printf("%s[%d] : miniupnpd recv event packet\n",__FUNCTION__,__LINE__);
#endif
				recv_event(p_event);
				p_event->state = 300;
				ret=0;
			}
		}
		/* process incoming HTTP connections */
		if(FD_ISSET(shttpl, &readset))
		{
			int shttp;
			socklen_t clientnamelen;
			struct sockaddr_in clientname;
			clientnamelen = sizeof(struct sockaddr_in);
			shttp = accept(shttpl, (struct sockaddr *)&clientname, &clientnamelen);
			if(shttp<0)
			{
				syslog(LOG_ERR, "accept(http): %m");
			}
			else
			{
				struct upnphttp * tmp = 0;
//				syslog(LOG_INFO, "HTTP connection from %s:%d",inet_ntoa(clientname.sin_addr),ntohs(clientname.sin_port) );
				/*if (fcntl(shttp, F_SETFL, O_NONBLOCK) < 0) {
					syslog(LOG_ERR, "fcntl F_SETFL, O_NONBLOCK");
				}*/
				/* Create a new upnphttp object and add it to
				 * the active upnphttp object list */
				tmp = New_upnphttp(shttp);
				if(tmp)
				{
					LIST_INSERT_HEAD(&upnphttphead, tmp, entries);
				}
				else
				{
					syslog(LOG_ERR, "New_upnphttp() failed");
					close(shttp);
				}
			}
		}
		/* delete finished HTTP connections */
		for(e = upnphttphead.lh_first; e != NULL; e = e->entries.le_next)
		{
			if(e->state >= 100)
			{
#ifdef DEBUG
				printf("%s[%d] : http connection over, need delete it\n",__FUNCTION__,__LINE__);
#endif
				LIST_REMOVE(e, entries);
				Delete_upnphttp(e);
			}
		}
		/* delet finish event obj */
		for(p_event = event_handlehead.lh_first; p_event != NULL; p_event = p_event->entries.le_next){
			if(  (p_event->state == -100) || (p_event->state == 300)){
#ifdef DEBUG
				printf("%s[%d] : upnp event over, need reset it\n",__FUNCTION__,__LINE__);
#endif
				p_event->state = 0;
				if(p_event->socket > 0){
					close(p_event->socket);
					p_event->socket=-1;//Changed the socket to -1 by Shearer Lu on 2009.07.23
				}
			}
		}
	}

shutdown:
	/* close out open sockets */
	while(upnphttphead.lh_first != NULL)
	{
		e = upnphttphead.lh_first;
		LIST_REMOVE(e, entries);
		Delete_upnphttp(e);
	}
	while(time_head.lh_first != NULL)
	{
		LIST_REMOVE(time_head.lh_first, entries);
	}
	close(sudp);
	close(shttpl);
	
	if(SendSSDPGoodbye(snotify) < 0)
	{
		syslog(LOG_ERR, "Failed to broadcast good-bye notifications");
	}
	close(snotify);

	return 0;
}	


