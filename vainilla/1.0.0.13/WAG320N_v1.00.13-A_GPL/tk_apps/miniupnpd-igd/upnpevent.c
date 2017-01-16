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
#include <linux/errno.h>
/* for BSD's sysctl */
#include <sys/param.h>
#include <sys/sysctl.h>
#include <errno.h>

/* unix sockets */
#include "maco.h"
#include "upnphttp.h"
#include "upnpdescgen.h"
#include "upnpsoap.h"
#include "upnphttp.h"
#include "upnpevent.h"
#include "upnphttp_func.h"
#include "minissdp.h"

char *my_strstr(const char *haystack, const char *needle, int mode)
{
	int i, len;
	
	if(mode == 0)
		return strstr(haystack,needle);
	if(mode == 1)
	{
		len = strlen(haystack) - strlen(needle);
		
		for( i = 0; i <= len; i++)
		{
			if(strncasecmp(haystack + i, needle, strlen(needle)) == 0)
				return (char *)(haystack + i);
		}
	}

	return NULL;
}

/* Creat enent handle, Just support simple event */
static void creat_event_handle(struct upnphttp * h, struct eventlisthead *pevent_handlehead, struct event_list *list)
{
	char * p;
	char *tmp, *t;
	char event_URL[128];
	char ip[sizeof("255.255.255.255")];
	char port[sizeof("65535")];
	char path[128];
	char time_out[20];
	int i;
	struct event_handle *p_event;

#ifdef DEBUG
	printf("%s[%d] : create new event handle obj\n",__FUNCTION__,__LINE__);
#endif


	p = h->req_buf;

      /*SUBSCRIBE /upnp/event/WANIPConn1 HTTP/1.1*/
	tmp = my_strstr(p,"SUBSCRIBE",1);
	tmp += strlen("SUBSCRIBE");

	/* Get event URL */
	while(*tmp ==' ')
		 tmp++;
	i = 0;
	while(*tmp != ' '&& (i < sizeof(event_URL) - 1) && *tmp )
	{
		event_URL[i++] = *tmp++;
	}
	event_URL[i] = 0;

	/* CALLBACK: <http://192.168.1.2:13043/upnp> */

	/* find URL */
	tmp = my_strstr(p,"Callback:",1);
	if(tmp == NULL)
		goto fail;
	tmp += strlen("Callback:");

	/* Get IP */
	t = tmp;
	tmp = my_strstr(t,"http://",1);
	if(tmp == NULL)
		goto fail;
	tmp += strlen("http://");
	i = 0;
	while(*tmp != ':' && (i < sizeof(ip) - 1) && *tmp)
	{
		ip[i++] = *tmp++;
	}
	ip[i] = '\0';
	if(*tmp != ':')
		goto fail;

	/* Get Port */
	i = 0;
	tmp ++;
	while((*tmp != '/') && (i < sizeof(port) - 1) && *tmp)
	{
		port[i++] = *tmp++;
	}
	port[i] = '\0';

	if(*tmp != '/')
		goto fail;

	/* Get Path */
	i = 0;
	tmp ++;
	while((*tmp != '>') && (i < sizeof(path) -1)&& *tmp)
	{
		path[i++] = *tmp++;
	}
	path[i] = '\0';

	/*get time_out*/
	tmp = my_strstr(p,"Timeout:",1);
	if(tmp == NULL)
		sprintf(time_out,"infinite");
	else
	{
		tmp += strlen("Timeout:");
		while(*tmp == ' ')
		   tmp++;
              i = 0;
		while((*tmp != '\r') && (i < sizeof(time_out) -1) && *tmp )
			time_out[i++] = *tmp++;
		time_out[i] = '\0';
		if(*tmp != '\r' )
			goto fail;
	}
#ifdef DEBUG
	printf("%s[%d] : event_URL=[%s]\n",__FUNCTION__,__LINE__,event_URL);
	printf("%s[%d] : ip=[%s]\n",__FUNCTION__,__LINE__,ip);
	printf("%s[%d] : port=[%s]\n",__FUNCTION__,__LINE__,port);
	printf("%s[%d] : path=[%s]\n",__FUNCTION__,__LINE__,path);
	printf("%s[%d] : time_out=[%s]\n",__FUNCTION__,__LINE__,time_out);
#endif
	p_event = (struct event_handle *)malloc(sizeof(struct event_handle));
	if(p_event == NULL)
		goto fail;
	memset(p_event,0,sizeof(struct event_handle));
	snprintf(p_event->event_URL, sizeof(p_event->event_URL), "%s", event_URL);
	snprintf(p_event->call_back_URL, sizeof(p_event->call_back_URL), "%s", path);
	snprintf(p_event->ip_addr, sizeof(p_event->ip_addr), "%s", ip);
	p_event->port = atoi(port);
	if(strstr("infinite",time_out) != NULL)
		p_event->time_out = -1;
	else
		p_event->time_out = atoi(time_out + strlen("Second-"));
	
	create_uuid(p_event->uuid);
	p_event->state = 0;
	p_event->socket = -1;
	p_event->creat_time_flag = time(NULL);
	p_event->seq = -1;
#ifdef DEBUG
	printf("%s[%d] : create event obj\n",__FUNCTION__,__LINE__);
	printf("%s[%d] : p_event->uuid=%s\n",__FUNCTION__,__LINE__,p_event->uuid);
	printf("%s[%d] : p_event->time_out=%d\n",__FUNCTION__,__LINE__,p_event->time_out);
#endif

	if(find_event_URL(list,p_event->event_URL) == NULL)
	{
		free(p_event);
		goto fail;
	}
	send_register_event_back(p_event,h);
	LIST_INSERT_HEAD(pevent_handlehead, p_event, entries);
	preapre_send_event(p_event);
#ifdef DEBUG
	printf("%s[%d] : add new event(ip=%s,URL=%s) to list\n",__FUNCTION__,__LINE__,p_event->ip_addr,p_event->event_URL);
#endif
	return ;

fail:
	Send501(h);
}

struct event_handle *find_event_uuid(struct eventlisthead *pevent_handlehead, char *uuid)
{
	struct event_handle *p_event;

	/* Check time */
	for(p_event = pevent_handlehead->lh_first; p_event != NULL; p_event = p_event->entries.le_next)
	{
		if(strcmp(p_event->uuid,uuid) == 0)
			return p_event;
	}

	return NULL;
}

struct event_handle *find_event_url(struct eventlisthead *pevent_handlehead, char *event_url)
{
	struct event_handle *p_event;

	/* Check time */
	for(p_event = pevent_handlehead->lh_first; p_event != NULL; p_event = p_event->entries.le_next)
	{
		if(strcmp(p_event->event_URL,event_url) == 0)
			return p_event;
	}

	return NULL;
}

void renew_service(struct upnphttp *h, struct eventlisthead *pevent_handlehead )
{
	char *p;
	char *tmp;
	int i;
	char buf[64]={0}, event_URL[128]={0};
	struct event_handle *p_event;
	p = h->req_buf;
	
	//Added by Shearer Lu on 2009.07.23
	//Parse event URL and use it to find the event in case that no uuid is found.
	//It is needed to pass the UPnP Certificate tool.
	
      /*SUBSCRIBE /upnp/event/WANIPConn1 HTTP/1.1*/
	tmp = my_strstr(p,"SUBSCRIBE",1);
	if(!tmp)
		return;
	tmp += strlen("SUBSCRIBE");
	/* Get event URL */
	while(*tmp ==' ')
		 tmp++;
	i = 0;
	while(*tmp != ' '&& (i < sizeof(event_URL) - 1) && *tmp )
	{
		event_URL[i++] = *tmp++;
	}
	event_URL[i] = 0;
	//Add End
	
#ifdef DEBUG
	printf("%s[%d] : renew event handle obj\n",__FUNCTION__,__LINE__);
#endif	
	if((tmp = strstr(p,"SID")) == NULL) {
		return ;
	}
	tmp += strlen("SID: uuid:");	
	i = -1;
	while(tmp[++i] != '\r' && i < 64 -1)
		buf[i] = tmp[i];
	buf[i] = 0;
	if((p_event = find_event_uuid(pevent_handlehead,buf)) == NULL && (p_event = find_event_url(pevent_handlehead,event_URL)) == NULL)
		return ;
	p_event->creat_time_flag = time(NULL);
	send_register_event_back(p_event,h);
}

struct event_list *find_event_URL(struct event_list *url_list, char *URL)
{
	int i = -1;
	
	while(url_list[++i].event_URL != NULL)
	{
		if(strcmp(url_list[i].event_URL, URL) == 0)
			return &url_list[i];
	}
	return NULL;
}

void send_register_event_back(struct event_handle *p_event, struct upnphttp *http)
{
	static const char content[] = "HTTP/1.1 200 OK\r\n"
//		"DATE: Thu, 01 Jan 1970 00:01:02 GMT\r\n"
		"Server: " MINIUPNPD_SERVER_STRING "\r\n"
		"SID: uuid:%s\r\n"
		"TIMEOUT: %s\r\n\r\n";
	int n;
	char time_out_str[20];
	char body[1024];

	if(p_event->time_out == -1)
		sprintf(time_out_str,"infinite");
	else
		sprintf(time_out_str,"Second-%d",p_event->time_out);
	sprintf(body,content,p_event->uuid,time_out_str);
	n = send(http->socket, body, strlen(body), 0);
	if(n < 0)
	{
		syslog(LOG_ERR, "Send501: send(http): %m");
	}
	CloseSocket_upnphttp(http);
}

void create_uuid(char *buf)
{
	int i = 0;
	unsigned char random[MAX_UUID_LENGTH];
	unsigned char tmp[MAX_UUID_LENGTH];
	int fd = open("/dev/urandom", O_RDONLY);
	
	read(fd,random,MAX_UUID_LENGTH);
	for(i = 0; i < MAX_UUID_LENGTH; i++)
		tmp[i] = random[i]%16;
	sprintf(buf,"%x%x%x%x%x%x%x%x-%x%x%x%x-%x%x%x%x-%x%x%x%x-%x%x%x%x%x%x%x%x%x%x%x%x",
		tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5],tmp[6],tmp[7],tmp[8],tmp[9],tmp[10],tmp[11],
		tmp[12],tmp[13],tmp[14],tmp[15],tmp[16],tmp[17],tmp[18],tmp[19],
		tmp[20],tmp[21],tmp[22],tmp[23],tmp[24],tmp[25],tmp[26],tmp[27],tmp[28],tmp[29],tmp[30],tmp[31]);
	close(fd);
}

void handle_subcribe(struct upnphttp *h, struct eventlisthead *pevent_handlehead, struct event_list *list)
{
	char *p;
	p = h->req_buf;
	
	if(strstr(p,"SID") != NULL)
		renew_service(h,pevent_handlehead);
	else
		creat_event_handle(h,pevent_handlehead,list);
}

void send_unsubcribe(struct upnphttp *h)
{
	static const char content[] = "HTTP/1.1 200 OK\r\n"
//		"DATE: Thu, 01 Jan 1970 00:01:02 GMT\r\n"
		"Server: " MINIUPNPD_SERVER_STRING "\r\n"
		"CONNECTION: close\r\n"
		"CONTENT-LENGTH: 41\r\n"
		"CONTENT-TYPE: text/html\r\n"
		"\r\n"
		"<html><body><h1>200 OK</h1></body></html>\r\n";

	int n = send(h->socket, content, sizeof(content) - 1, 0);
	if(n < 0)
	{
		syslog(LOG_ERR, "unsubcribe: send(http)");
	}
	CloseSocket_upnphttp(h);
}

void handle_unsubcribe(struct upnphttp *h, struct eventlisthead *pevent_handlehead, struct event_list *list)
{
	char *p;
	char *tmp;
	int i;
	char buf[64];
	struct event_handle *p_event;
	p = h->req_buf;

	if((tmp = strstr(p,"SID")) == NULL)
	{
		Send501(h);
		return ;
	}
	tmp += strlen("SID: uuid:");	
	i = -1;
	while(tmp[++i] != '\r' && i < 64 -1)
		buf[i] = tmp[i];
	buf[i] = 0;
	if((p_event = find_event_uuid(pevent_handlehead,buf)) == NULL)
		return ;
	send_unsubcribe(h);
	LIST_REMOVE(p_event, entries);
	Delete_event(p_event);
}

void Delete_event(struct event_handle *event)
{
	if(event)
	{
		if(event->socket > 0)
			close(event->socket);
		free(event);
	}
}

void preapre_send_event(struct  event_handle *p_event)
{
	struct sockaddr_in client_addr;
	int flags;
	int n;

	p_event->socket = socket(AF_INET,SOCK_STREAM,0);
	p_event->init_time_flag = time(NULL);
	p_event->state = 100;/* init */
	p_event->seq ++;
	flags = fcntl(p_event->socket ,F_GETFL,0);
	fcntl(p_event->socket ,F_SETFL, flags|O_NONBLOCK);
	bzero(&client_addr,sizeof(client_addr));
	inet_pton(AF_INET,p_event->ip_addr,&client_addr.sin_addr);
	client_addr.sin_family = AF_INET;
	client_addr.sin_port = htons(p_event->port);
	n = connect(p_event->socket,(struct sockaddr *)&client_addr,sizeof(struct sockaddr_in));
	if(n == EINPROGRESS){
		p_event->state = -100;/* connect error */
	}
}

int send_event_notify(struct event_handle *p_event, struct event_list *url_list )
{
	static const char notify[]="NOTIFY /%s HTTP/1.1\r\n"
						"HOST: %s:%d\r\n"
						"CONTENT-TYPE: text/xml\r\n"
						"CONTENT-LENGTH: %d\r\n"
						"NT: upnp:event\r\n"
						"NTS: upnp:propchange\r\n"
						"SID: uuid:%s\r\n"
						"SEQ: %d\r\n"
						"\r\n";

	char head[4096];
	char body[2048];
	struct event_list *list=NULL;
	int bodylen;
	int ret=0;
	
	list = find_event_URL( url_list, p_event->event_URL);
	if(!list)
		return -1;
	bodylen = list->gen_xml(body);
	sprintf( head, notify, p_event->call_back_URL, p_event->ip_addr,
				p_event->port,bodylen,p_event->uuid,p_event->seq);
	strcat(head,body);
	ret=send(p_event->socket, head, strlen(head), 0);
	return ret;
}

int recv_event(struct event_handle *p_event)
{
	char buf[1024];
	int len=0;
	
	len = recv(p_event->socket,buf,1024,0);
	if(len > 0)
	{
		buf[len] = 0;
#ifdef DEBUG
		printf("%s[%d] : recv packet %s",__FUNCTION__,__LINE__,buf);
#endif
	}
	else
	{
		if(errno==EBADF){
			p_event->state = 0;
			close(p_event->socket);
			p_event->socket=-1;
		}
	}
	return len;
}

