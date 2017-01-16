/* dhcpd.c
 *
 * Moreton Bay DHCP Server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *			Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>

#include "debug.h"
#include "dhcpd.h"
#include "arpping.h"
#include "socket.h"
#include "options.h"
#include "files.h"
#include "leases.h"
#include "packet.h"
#include "serverpacket.h"
#include "pidfile.h"
#include "nvram.h"
/* globals */

struct dhcpOfferedAddr *leases[MAX_INTERFACES][MAX_SERVERS_PER_IF];
struct server_config_t server_config[MAX_INTERFACES][MAX_SERVERS_PER_IF];
int no_of_ifaces = 0;
int no_of_servers[MAX_INTERFACES];
char *nvram_data;

extern struct reserve_ip check;

#if 0
/*
* change mac from format 00:0c:76:23:9f:55 to net order
*/
static int mac_aton(unsigned char *mac_input, unsigned char* mac_net)
{
	int mac_arr[6];
	int len = 6;

	if(mac_input == NULL || mac_net == NULL)
		return -1;
	sscanf(mac_input, "%02x:%02x:%02x:%02x:%02x:%02x", mac_arr, mac_arr + 1, mac_arr + 2, mac_arr + 3, mac_arr + 4, mac_arr + 5);

	while(len--)
	{
		*(mac_net + len) = mac_arr[len];

	}

	return 0;
}
#endif

/*
* find vid by mac address through file /proc/mactbl
*/
u_int32_t find_server_vid_by_chaddr(u_int8_t *chaddr)
{
	FILE *fp;
	char buffer[80];
	char tmp[20];
	char get_tmp[20];
	int vid;
	char *line;
	char *p;
	char *wireless;


#ifdef _WIFI_BR0_
	int wireless_vid;
	wireless = nvram_get("wireless_vlan");
	if(wireless == NULL)
		wireless_vid = 1;
	else{
		wireless_vid = atoi(wireless);
		free(wireless);
	}
#endif

	sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x", *chaddr, *(chaddr + 1),
			*(chaddr + 2), *(chaddr + 3), *(chaddr + 4), *(chaddr + 5));

	if(!(fp = fopen(SERVER_VID_DIR, "r"))){
		LOG(LOG_ERR, "unable to open mactbl file: %s\n", SERVER_VID_DIR);
		return 0;
	}
	fgets(buffer, sizeof(buffer), fp); /* eat the first line */
	while(fgets(buffer, sizeof(buffer), fp))
	{
		p = strchr(buffer, '\n');
		if(p)
			*p = '\0';
		line = buffer;
		sscanf(line, " \t%s \t%d", get_tmp, &vid);
		if(strcasecmp(get_tmp, tmp))
		{
#ifdef _WIFI_BR0_
			vid = wireless_vid; //reset vid value
#endif
			continue;
		}

		break;
	}

	fclose(fp);
	return vid;
}

/*
* find dhcp server index by vid
*/

int server_vid_to_index(int ifid, int vid)
{
	int i;

	if(!vid) {
	    return 0;
	}

	for(i = 0; i < no_of_servers[ifid]; i++)
	{

		if((int)server_config[ifid][i].vid == vid)
		{
			if(server_config[ifid][i].enable == 1)
				return i;
			else
				return -1;
		}
	}

	return -1;
}


/* Exit and cleanup */
static void exit_server(int retval, int ifid)
{
	int i;

	for(i = 0; i < no_of_servers[ifid]; i++)
		pidfile_delete(server_config[ifid][i].pidfile);
	CLOSE_LOG();
	exit(retval);
}


/* SIGTERM handler */
static void udhcpd_killed(int sig)
{
    int i;

	sig = 0;
	LOG(LOG_INFO, "Received SIGTERM");
	for(i=0; i<MAX_INTERFACES; i++) {
	    exit_server(0, i);
	}
}

/* -- Jeff Sun -- Apr.23.2005 -- add here for make ipaddr expire */
static void expire_action(u_int32_t ipaddr)
{
    int i;
    int k;
    unsigned int j;

    for(i=0;i<no_of_ifaces;i++)
	{
		for(k = 0; k < no_of_servers[i]; k++)
		{
			if(server_config[i][0].active == FALSE)
				continue;

    			for(j=0;j<server_config[i][k].max_leases;j++)
    				if(leases[i][k][j].yiaddr == ipaddr)
				{
					leases[i][k][j].expires = time(0);
				}
		}
    }
}
/* SIGUSR1 handler */
static void do_expire(int sig)
{
    FILE *fp;
    char ip[20];
    char *pp;
    struct in_addr addr;

    sig = 0;

    if(access("/tmp/dhcpd.delete",F_OK)==0)
    {
        if( (fp=fopen("/tmp/dhcpd.delete","r")) != NULL )
        {
            while(fgets(ip,20,fp)!=NULL)
            {

                if( (pp=strchr(ip,'#')) != NULL )
                {
                    *pp='\0';
                    inet_aton(ip, &addr);
                    expire_action(addr.s_addr);
                }
            }
            fclose(fp);
        }
        system("/bin/rm -rf /tmp/dhcpd.delete");
    }
}

#ifdef COMBINED_BINARY
int udhcpd(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	fd_set rfds;
	struct timeval tv;
	int server_socket[MAX_INTERFACES];
	int bytes, retval;
	struct dhcpMessage packet;
	unsigned char *state;
	unsigned char *server_id, *requested;
	u_int32_t server_id_align, requested_align;
	u_int32_t timeout_end[MAX_INTERFACES][MAX_SERVERS_PER_IF];
	struct option_set *option;
	struct dhcpOfferedAddr *lease;
	int pid_fd;
	int i;
	int j;
	u_int32_t server_vid;

	OPEN_LOG("udhcpd");
	LOG(LOG_INFO, "udhcp server (v%s) started", VERSION);

	for (i = 0; i < MAX_INTERFACES; i++)
		for( j= 0; j < no_of_servers[i]; j++)
			memset(&server_config[i][j], 0, sizeof(struct server_config_t));

	if (argc < 2)
		read_config(DHCPD_CONF_FILE);
	else 
		read_config(argv[1]);

	if (no_of_ifaces == 0)
		exit(0);

	for (i = 0; i < no_of_ifaces; i++)
		for( j= 0; j < no_of_servers[i]; j++)
		{
			pid_fd = pidfile_acquire(server_config[i][j].pidfile);
			pidfile_write_release(pid_fd);

			if ((option = find_option(server_config[i][j].options, DHCP_LEASE_TIME)))
			{
				memcpy(&server_config[i][j].lease, option->data + 2, 4);
				server_config[i][j].lease = ntohl(server_config[i][j].lease);
			}
			else 
				server_config[i][j].lease = LEASE_TIME;

			leases[i][j] = malloc(sizeof(struct dhcpOfferedAddr) * server_config[i][j].max_leases);
			memset(leases[i][j], 0, sizeof(struct dhcpOfferedAddr) * server_config[i][j].max_leases);

			read_leases(server_config[i][j].lease_file, i, j);

			if (read_interface(server_config[i][j].interface, &server_config[i][j].ifindex,
				   &server_config[i][j].server, server_config[i][j].arp) < 0)
				server_config[i][j].active = FALSE;
			else
				server_config[i][j].active = TRUE;

#ifndef DEBUGGING
			pid_fd = pidfile_acquire(server_config[i][j].pidfile); /* hold lock during fork. */
			/* cfgmr req: do not fork */
			/*
			if (daemon(0, 0) == -1) {
				perror("fork");
				exit_server(1, i);
			}
			*/

			pidfile_write_release(pid_fd);
#endif
			signal(SIGUSR2, reconfig_dhcpd);
			signal(SIGUSR1, do_expire);
			signal(SIGTERM, udhcpd_killed);
		}

//	return 0;
	for (i = 0; i < no_of_ifaces; i++)
	{
		server_socket[i] = -1;
		for( j= 0; j < no_of_servers[i]; j++)
		{
			timeout_end[i][j] = time(0) + server_config[i][j].auto_time;
			LOG(LOG_INFO, "interface: %s, start : %x end : %x\n", server_config[i][j].interface, server_config[i][j].start, server_config[i][j].end);
		}
	}

	//LOG(LOG_INFO, "%s %d\n", __FUNCTION__, __LINE__);
	while(1)
	{ /* loop until universe collapses */
		for (i = 0; i < no_of_ifaces; i++)
		{
			if (server_config[i][0].active == FALSE)
					continue;

			if (server_socket[i] < 0)
				if ((server_socket[i] = listen_socket(INADDR_ANY, SERVER_PORT, server_config[i][0].interface)) < 0)
				{
					LOG(LOG_ERR, "FATAL: couldn't create server socket, %s", strerror(errno));
					exit_server(0, i);
				}

			FD_ZERO(&rfds);
			FD_SET(server_socket[i], &rfds);
			for( j= 0; j < no_of_servers[i]; j++)
				if (server_config[i][j].auto_time)
				{
					tv.tv_sec = timeout_end[i][j] - time(0);
					if (tv.tv_sec <= 0)
					{
						tv.tv_sec = server_config[i][j].auto_time;
						timeout_end[i][j] = time(0) + server_config[i][j].auto_time;
						write_leases(i, j);
					}
					tv.tv_usec = 0;
				}
			retval = select(server_socket[i] + 1, &rfds, NULL, NULL, server_config[i][0].auto_time ? &tv : NULL);

			if (retval == 0)
			{
				for( j= 0; j < no_of_servers[i]; j++)
				{
					write_leases(i, j);
					timeout_end[i][j] = time(0) + server_config[i][0].auto_time;
				}
				continue;
			}
			else if (retval < 0)
			{
				DEBUG(LOG_INFO, "error on select");
				continue;
			}

//			LOG(LOG_INFO, "%s %d\n", __FUNCTION__, __LINE__);
			if ((bytes = get_packet(&packet, server_socket[i])) < 0) { /* this waits for a packet - idle */
				if (bytes == -1 && errno != EINTR)
				{
					DEBUG(LOG_INFO, "error on read, %s, reopening socket", strerror(errno));
					close(server_socket[i]);
					server_socket[i] = -1;
				}
				continue;
			}

//			LOG(LOG_INFO, "%s %d\n", __FUNCTION__, __LINE__);
			if ((state = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL)
			{
				DEBUG(LOG_ERR, "couldn't get option from packet, ignoring");
				continue;
			}

			/* ADDME: look for a static lease */
//			LOG(LOG_INFO, "%s %d\n", __FUNCTION__, __LINE__);
			server_vid = find_server_vid_by_chaddr(packet.chaddr);
//			LOG(LOG_INFO, "%s %d\n", __FUNCTION__, __LINE__);
			j= server_vid_to_index(i, server_vid);
//			LOG(LOG_INFO, "%s %d j is <%d>\n", __FUNCTION__, __LINE__, j);
			if(j == -1)
				continue;
			lease = find_lease_by_chaddr(packet.chaddr, i, j);
			LOG(LOG_INFO, "INDEX = %d SERVERINDEX = %d packet.chaddr = %s", i, j, packet.chaddr);
			switch (state[0])
			{
				case DHCPDISCOVER:
					DEBUG(LOG_INFO,"received DISCOVER");
					if (sendOffer(&packet, i, j) < 0)
					{
						LOG(LOG_ERR, "send OFFER failed");
					}
					break;

				case DHCPREQUEST:
				{
					DEBUG(LOG_INFO, "received REQUEST");
					int check_res = 0;
					requested = get_option(&packet, DHCP_REQUESTED_IP);
					server_id = get_option(&packet, DHCP_SERVER_ID);
					if (requested) memcpy(&requested_align, requested, 4);
					if (server_id) memcpy(&server_id_align, server_id, 4);

					check_res = isreserve_mac(packet.chaddr);
					
					if (lease)
					{ /*ADDME: or static lease */
						if(check_res)
						{
							if((requested && inet_addr(check.res_ip)==requested_align)
							   || (packet.ciaddr==inet_addr(check.res_ip)))
							;
							else
							{
								sendNAK(&packet, i, j);
								break;
							}
						}
						if (server_id)
						{
							/* SELECTING State */
							DEBUG(LOG_INFO, "server_id = %08x", ntohl(server_id_align));
							if (server_id_align == server_config[i][j].server && requested &&
					    		requested_align == lease->yiaddr)
					    	{
								sendACK(&packet, lease->yiaddr, i, j);
							}
						}
						else
						{
							if (requested)
							{
								/* INIT-REBOOT State */
								if (lease->yiaddr == requested_align)
									sendACK(&packet, lease->yiaddr, i, j);
								else sendNAK(&packet, i, j);
							}
							else
							{
								/* RENEWING or REBINDING State */
								if ((lease->yiaddr == packet.ciaddr)
									&& (packet.ciaddr <= server_config[i][j].end)
									&& (packet.ciaddr >= server_config[i][j].start))
									sendACK(&packet, lease->yiaddr, i, j);
								else
								{
									/* don't know what to do!!!! */
									sendNAK(&packet, i, j);
								}
							}
						}
                 	/* what to do if we have no record of the client */
					}
					else if (server_id)
					{
						/* SELECTING State */
						sendNAK(&packet,i, j);
					}
					else if (requested)
					{
					               /* INIT-REBOOT State */
						if ((lease = find_lease_by_yiaddr(requested_align, i, j)))
						{
							if (lease_expired(lease))
							{
					    		/* probably best if we drop this lease */
								memset(lease->chaddr, 0, 16);
								/* make some contention for this address */
							}
							else
							{
								sendNAK(&packet,i,j);
						  	}
			    		}
			    		else if(requested_align < server_config[i][j].start ||
					      		requested_align > server_config[i][j].end)
						{
					 		sendNAK(&packet,i,j);
						}
						else
						{
							/* else remain silent */
							sendNAK(&packet,i,j);
						}
		     		}
		     		else
					{
						/* RENEWING or REBINDING State */
						sendNAK(&packet,i,j);

					}
					break;
				}
				case DHCPDECLINE:
					DEBUG(LOG_INFO,"received DECLINE");
					if (lease)
					{
						memset(lease->chaddr, 0, 16);
						lease->expires = time(0) + server_config[i][j].decline_time;
					}
					break;

				case DHCPRELEASE:
					DEBUG(LOG_INFO,"received RELEASE");
					if (lease) lease->expires = time(0);
					break;

				case DHCPINFORM:
					DEBUG(LOG_INFO,"received INFORM");
					send_inform(&packet, i, j);
					break;

				default:
					LOG(LOG_WARNING, "unsupported DHCP message (%02x) -- ignoring", state[0]);
			}
		}
	}
	return 0;
}

