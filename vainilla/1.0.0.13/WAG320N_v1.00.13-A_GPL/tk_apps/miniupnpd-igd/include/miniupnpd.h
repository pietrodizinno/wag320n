/* 2007.8.3*/
#ifndef __MINIUPNPD_H__
#define __MINIUPNPD_H__

/* structure containing variables used during "main loop"
 * that are filled during the init */
struct runtime_vars {
	int n_add_listen_addr;
	const char * add_listen_addr[MAX_ADD_LISTEN_ADDR];
	int port;	/* HTTP Port */
	int notify_interval;	/* seconds between SSDP announces */
	char listen_addr[MAX_IP_ADDR];
};

extern int upnp_init(struct  service_type_uuid *,char *,char *,char *,struct method *,struct http_desc *,int );

int miniupnp_deamon(struct runtime_vars *, struct event_list *);

extern time_t startup_time;
#endif
