/*******************************************************
 *              WAG200GV2 igd_upnpd.  
 *      This file support upnp event .
 *      CopyRight 2007 @ Sercomm By Oliver.Hao.
 *******************************************************/

#ifndef __UPNPEVENT_H_
#define __UPNPEVENT_H_

struct  event_handle{
	int socket;
	int state;/* 0:registed; 100:init; -100:error; 200:connect sucess; 300:finish */
	char event_URL[MAX_URL_LEN];
	char call_back_URL[MAX_URL_LEN];
	char ip_addr[MAX_IP_ADDRESS];
	char uuid[MAX_UUID_LENGTH+1];
	int port;
	int seq;
	int time_out;
	time_t creat_time_flag;
	time_t init_time_flag;
	time_t send_time_flag;
	LIST_ENTRY(event_handle) entries;
};

struct event_list{
	const char * event_URL; 
	int (*check_event)(void );
	int (*gen_xml)(char *);
} ;

struct event_table{
		char *name;
		int (*get_value)(char *);
} ;

#include <sys/queue.h> 
LIST_HEAD(eventlisthead, event_handle);

extern struct event_list EVENT_URL_LIST[];

struct event_list *find_event_URL(struct event_list *, char *);

void Delete_event(struct event_handle *);

void handle_subcribe(struct upnphttp *, struct eventlisthead *, struct event_list *);

void handle_unsubcribe(struct upnphttp *, struct eventlisthead *, struct event_list *);

void send_register_event_back(struct event_handle *, struct upnphttp *);

void create_uuid(char *);

void preapre_send_event(struct  event_handle *);

int recv_event(struct event_handle *);

int send_event_notify(struct event_handle *, struct event_list * );

#endif

