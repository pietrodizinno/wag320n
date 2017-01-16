/* dhcpd.c
 *
 * udhcp DHCP client
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
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

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <fcntl.h>

#include "dhcpd.h"
#include "dhcpc.h"
#include "options.h"
#include "clientpacket.h"
#include "packet.h"
#include "script.h"
#include "socket.h"
#include "debug.h"
#include "pidfile.h"
#include "nvram.h"
#include "arpping.h"

static int state;
static u_int32_t requested_ip; /* = 0 */
static u_int32_t server_addr;
static u_int32_t timeout;
static int packet_num; /* = 0 */
static int fd;
u_int32_t OldIP = 0;
u_int32_t OldGW = 0;
u_int32_t NewGW = 0;

#define LISTEN_NONE 0
#define LISTEN_KERNEL 1
#define LISTEN_RAW 2
static int listen_mode;

#define DEFAULT_SCRIPT	"/usr/share/udhcpc/default.script"

struct client_config_t client_config = {
	/* Default options. */
	abort_if_no_lease: 0,
	foreground: 0,
	quit_after_lease: 0,
	interface: LAN_INTERFACE,
	pidfile: NULL,
	script: DEFAULT_SCRIPT,
	clientid: NULL,
	hostname: NULL,
	ifindex: 0,
	arp: "\0\0\0\0\0\0",		/* appease gcc-3.0 */
};

#ifdef AUTO_IP
int in_auto_ip=0, pure_nas=0;

extern void config_auto_ip(char *ip_addr, char *netmask);
#endif

#ifdef AUTO_IP
int getlocalparm(char haddr[6])
{
	int sockd;
	struct ifreq ifr;

	sockd = socket(AF_INET, SOCK_STREAM, 0); /* IP = 0 */
	if (sockd == -1) {
		return -1;
	}
	/* hardware address */
	strcpy(ifr.ifr_name, client_config.interface);
	if (ioctl(sockd, SIOCGIFHWADDR, &ifr) < 0)
	{
		close(sockd);
		return -1;
	}
	memcpy(haddr,ifr.ifr_hwaddr.sa_data,6);
	close(sockd);

	return 0;
}
typedef	unsigned	short	WORD;
#define ULONG     unsigned int          /** 4 bytes **/
#define xchgs(x) x //(WORD)((((WORD)((WORD)x)&0x0ff)<<8)|(((WORD)((WORD)x)&0x0ff00)>>8))
#define xchgl(x) x //((ULONG)(xchgs((WORD)(x & 0x0000FFFF))<<16) | (ULONG)(xchgs((WORD)((x & 0xFFFF0000)>>16))))

int GetRandomIPAddress(struct in_addr *auto_ip)
{
	struct in_addr low_ip, high_ip;
	unsigned long ip_num=0,random_num=0, low_ip_num=0, high_ip_num=0;
	int ret=0,flag=0;
	char hw_addr[6]={0},ip_str[16]={0};
	u_char sender_mac[6];
	
	ret=getlocalparm(hw_addr);
	if(ret)
		return -1;
	ret=inet_aton("169.254.1.0", &low_ip);
	if(!ret)
		return -1;
	ret=inet_aton("169.254.254.255", &high_ip);
	if(!ret)
		return -1;
	flag=0;
	low_ip_num=xchgl(low_ip.s_addr);
	high_ip_num=xchgl(high_ip.s_addr);
	ip_num=high_ip_num-low_ip_num-512;
	while(1){
		random_num=random()%ip_num;
		auto_ip->s_addr=xchgl((low_ip_num+256+random_num));
		if(flag){				    
			if(arpping(inet_addr(inet_ntoa(*auto_ip)), inet_addr(ip_str), hw_addr, client_config.interface, sender_mac))
				break;
		}
		else{
			if(arpping(inet_addr(inet_ntoa(*auto_ip)), inet_addr(inet_ntoa(*auto_ip)), hw_addr, client_config.interface, sender_mac))
				break;		
		}
	}
	return 0;
}
#endif

static void print_usage(void)
{
	printf(
"Usage: udhcpcd [OPTIONS]\n\n"
"  -c, --clientid=CLIENTID         Client identifier\n"
"  -H, --hostname=HOSTNAME         Client hostname\n"
"  -f, --foreground                Do not fork after getting lease\n"
"  -i, --interface=INTERFACE       Interface to use (default: "LAN_INTERFACE")\n"
"  -n, --now                       Exit with failure if lease cannot be\n"
"                                  immediately negotiated.\n"
"  -p, --pidfile=file              Store process ID of daemon in file\n"
"  -q, --quit                      Quit after obtaining lease\n"
"  -r, --request=IP                IP address to request (default: none)\n"
"  -s, --script=file               Run file at dhcp events (default:\n"
"                                  " DEFAULT_SCRIPT ")\n"
"  -v, --version                   Display version\n"
	);
}

void nvram_safe_free(char *name)
{
    if(name!=NULL && strcmp(name,"")!=0)
        free(name);
}

/* just a little helper */
static void change_mode(int new_mode)
{
	DEBUG(LOG_INFO, "entering %s listen mode",
		new_mode ? (new_mode == 1 ? "kernel" : "raw") : "none");
	close(fd);
	fd = -1;
	listen_mode = new_mode;
}


/* SIGUSR1 handler (renew) */
static void renew_requested(int sig)
{
	sig = 0;
	LOG(LOG_INFO, "Received SIGUSR1");
	if (state == BOUND || state == RENEWING || state == REBINDING ||
	    state == RELEASED) {
	    	change_mode(LISTEN_KERNEL);
		packet_num = 0;
		state = RENEW_REQUESTED;
	}

	if (state == RELEASED) {
		change_mode(LISTEN_RAW);
		state = INIT_SELECTING;
	}

	/* Kill any timeouts because the user wants this to hurry along */
	timeout = 0;
}


/* SIGUSR2 handler (release) */
static void release_requested(int sig)
{
	sig = 0;
	LOG(LOG_INFO, "Received SIGUSR2");
	/* send release packet */
	if (state == BOUND || state == RENEWING || state == REBINDING) {
		send_release(server_addr, requested_ip); /* unicast */
		run_script(NULL, "deconfig");
		OldIP = 0;
	}

	change_mode(LISTEN_NONE);
	state = RELEASED;
	timeout = 0x7fffffff;
	OldGW = 0;
	OldIP = 0;
#ifdef LED
    if(access("/proc/adsl", F_OK)==0)
        system("/bin/echo \"0\">/proc/adsl");
    sleep(1);
    system("/bin/echo \"io\">/proc/led");
#endif /* LED */
}


/* Exit and cleanup */
static void exit_client(int retval)
{
	pidfile_delete(client_config.pidfile);
	CLOSE_LOG();
	system("/bin/rm -rf /tmp/wan_dhcp_server");
	exit(retval);
}


/* SIGTERM handler */
static void terminate(int sig)
{
	sig = 0;
	LOG(LOG_INFO, "Received SIGTERM");
	exit_client(0);
}


#if 1
static void background(void)
{
	int pid_fd;
	if (client_config.quit_after_lease) {
		exit_client(0);
	} else if (!client_config.foreground) {
		pid_fd = pidfile_acquire(client_config.pidfile); /* hold lock during fork. */
		if (daemon(0, 0) == -1) {
			perror("fork");
			exit_client(1);
		}
		client_config.foreground = 1; /* Do not fork again. */
		pidfile_write_release(pid_fd);
	}
}
#endif

#ifdef COMBINED_BINARY
int udhcpc(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	unsigned char *temp, *message;
	u_int32_t t1 = 0, t2 = 0, xid = 0;
	u_int32_t start = 0, lease;
	fd_set rfds;
	int retval;
	struct timeval tv;
	int c, len;
	struct dhcpMessage packet;
	struct in_addr temp_addr,serv_addr;
	int pid_fd;
	time_t now;
    	int fd_flag;
   	FILE *record_file;
   	pid_t pid;
    char *ethwan_enable = nvram_get("ethwan_enable");
    char *ethwan_encapmod = nvram_get("ethwan_encapmod");

#ifdef AUTO_IP
	char *pPureNasMode=NULL;

	pPureNasMode = nvram_get("pure_nas_mode");
	if(pPureNasMode && !strcmp(pPureNasMode, "yes")){
		pure_nas=1;
		client_config.abort_if_no_lease=1;
	}
#endif
	if((record_file=fopen("/tmp/udhcpc.pid","w"))!=NULL) {
  		pid=getpid();
  		fprintf(record_file,"%d\n",pid);
  		fflush(record_file);
  		fclose(record_file);
	}

	static struct option options[] = {
		{"clientid",	required_argument,	0, 'c'},
		{"foreground",	no_argument,		0, 'f'},
		{"hostname",	required_argument,	0, 'H'},
		{"help",	no_argument,		0, 'h'},
		{"interface",	required_argument,	0, 'i'},
		{"now", 	no_argument,		0, 'n'},
		{"pidfile",	required_argument,	0, 'p'},
		{"quit",	no_argument,		0, 'q'},
		{"request",	required_argument,	0, 'r'},
		{"script",	required_argument,	0, 's'},
		{"version",	no_argument,		0, 'v'},
		{0, 0, 0, 0}
	};
	/* get options */
	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "c:fH:hi:np:qr:s:v", options, &option_index);
		if (c == -1) break;

		switch (c) {
		case 'c':
			len = strlen(optarg) > 255 ? 255 : strlen(optarg);
			if (client_config.clientid) free(client_config.clientid);
			client_config.clientid = malloc(len + 2);
			client_config.clientid[OPT_CODE] = DHCP_CLIENT_ID;
			client_config.clientid[OPT_LEN] = len;
			client_config.clientid[OPT_DATA] = '\0';
			strncpy((char *)client_config.clientid + 3, optarg, len - 1);
			break;
		case 'f':
			client_config.foreground = 1;
			break;
		case 'H':
			len = strlen(optarg) > 255 ? 255 : strlen(optarg);
			if (client_config.hostname) free(client_config.hostname);
			client_config.hostname = malloc(len + 2);
			client_config.hostname[OPT_CODE] = DHCP_HOST_NAME;
			client_config.hostname[OPT_LEN] = len;
			strncpy(client_config.hostname + 2, optarg, len);
			break;
		case 'h':
			print_usage();
			return 0;
		case 'i':
			client_config.interface =  optarg;
			break;
		case 'n':
			client_config.abort_if_no_lease = 1;
			break;
		case 'p':
			client_config.pidfile = optarg;
			break;
		case 'q':
			client_config.quit_after_lease = 1;
			break;
		case 'r':
			requested_ip = inet_addr(optarg);
			break;
		case 's':
			client_config.script = optarg;
			break;
		case 'v':
			printf("udhcpcd, version %s\n\n", VERSION);
			exit_client(0);
			break;
		}
	}

	OPEN_LOG("udhcpc");
	LOG(LOG_INFO, "udhcp client (v%s) started", VERSION);

	pid_fd = pidfile_acquire(client_config.pidfile);
	pidfile_write_release(pid_fd);

	if (read_interface(client_config.interface, &client_config.ifindex,
			   NULL, client_config.arp) < 0)
    {
#ifdef LED
        system("/bin/echo i0>/proc/led");
        if(access("/proc/adsl", F_OK)==0)
            system("/bin/echo \"0\">/proc/adsl");
#endif /* LED */
		exit_client(1);
	}

	if (!client_config.clientid) {
		client_config.clientid = malloc(6 + 3);
		client_config.clientid[OPT_CODE] = DHCP_CLIENT_ID;
		client_config.clientid[OPT_LEN] = 7;
		client_config.clientid[OPT_DATA] = 1;
		memcpy(client_config.clientid + 3, client_config.arp, 6);
	}

	/* setup signal handlers */
	signal(SIGUSR1, renew_requested);
	signal(SIGUSR2, release_requested);
	signal(SIGTERM, terminate);

	state = INIT_SELECTING;
	run_script(NULL, "deconfig");
	change_mode(LISTEN_RAW);

	for (;;) {

		tv.tv_sec = timeout - time(0);
		tv.tv_usec = 0;
		FD_ZERO(&rfds);

//fix when get real time,dhcp will send 13 dhcp reguest packet.
if(start<1000000&&time(NULL)>1000000)
	start=time(0);
		if (listen_mode != LISTEN_NONE && fd < 0) {
			if (listen_mode == LISTEN_KERNEL)
				fd = listen_socket(INADDR_ANY, CLIENT_PORT, client_config.interface);
			else
				fd = raw_socket(client_config.ifindex);
			if (fd < 0) {
				LOG(LOG_ERR, "FATAL: couldn't listen on socket, %s", strerror(errno));
#ifdef LED
				system("/bin/echo i0>/proc/led");
                if(access("/proc/adsl", F_OK)==0)
                    system("/bin/echo \"0\">/proc/adsl");
#endif /* LED */
				exit_client(0);
			}
		}
		if (fd >= 0) FD_SET(fd, &rfds);
		
		if (tv.tv_sec > 0) {
			DEBUG(LOG_INFO, "Waiting on select...\n");

            /* set socket to non-blocking operation */
            if ((fd_flag = fcntl(fd, F_GETFL, 0)) >= 0) {
                fcntl(fd, F_SETFL, fd_flag | O_NONBLOCK);
            }
			retval = select(fd + 1, &rfds, NULL, NULL, &tv);
		} else retval = 0; /* If we already timed out, fall through */

		now = time(0);
		if (retval == 0) {
			/* timeout dropped to zero */
			switch (state) {
			case INIT_SELECTING:
				if (packet_num < 3) {
					if (packet_num == 0)
						xid = random_xid();

					/* send discover packet */
					send_discover(xid, requested_ip); /* broadcast */

					timeout = now + ((packet_num == 2) ? 10 : 2);
					packet_num++;
				} else {
#ifdef LED
				    system("/bin/echo i0>/proc/led");
                    if(access("/proc/adsl", F_OK)==0)
                        system("/bin/echo \"0\">/proc/adsl");
#endif /* LED */
					if (client_config.abort_if_no_lease
					#ifdef AUTO_IP
					 && pure_nas && !in_auto_ip
					#endif					 
					 ) {
					#ifdef AUTO_IP
						struct in_addr auto_ip;
						int ret=0;
						
						in_auto_ip=1;
					#endif		
						LOG(LOG_INFO, "No lease, failing.");		
					#ifndef AUTO_IP      
						exit_client(1);
					#else
						if(pure_nas){
							ret=GetRandomIPAddress(&auto_ip);
							if(ret)
								config_auto_ip("192.168.1.100", "255.255.255.0");
							else
								config_auto_ip(inet_ntoa(auto_ip), "255.255.0.0");
							printf("Use AUTO IP Address: %s.\n", inet_ntoa(auto_ip));
							background();
						}
					}
					#endif
					/* wait to try again */
					packet_num = 0;
					timeout = now + 35; // changed the time from 60 to 35 seconds
				}
				break;
			case RENEW_REQUESTED:
			case REQUESTING:
				if (packet_num < 3) {
					/* send request packet */
					if (state == RENEW_REQUESTED)
						send_renew(xid, server_addr, requested_ip); /* unicast */
					else send_selecting(xid, server_addr, requested_ip); /* broadcast */

					timeout = now + ((packet_num == 2) ? 10 : 2);
					packet_num++;
					
				} else {
#ifdef LED
				    system("/bin/echo i0>/proc/led");
                    if(access("/proc/adsl", F_OK)==0)
                        system("/bin/echo \"0\">/proc/adsl");
#endif
					/* timed out, go back to init state */
					if (state == RENEW_REQUESTED)
					    run_script(NULL, "deconfig");
					state = INIT_SELECTING;
					timeout = now;
					packet_num = 0;
					change_mode(LISTEN_RAW);
				}
				break;
			case BOUND:
				/* Lease is starting to run out, time to enter renewing state */
				state = RENEWING;
				change_mode(LISTEN_KERNEL);
				DEBUG(LOG_INFO, "Entering renew state");
				/* fall right through */
			case RENEWING:
				/* Either set a new T1, or enter REBINDING state */
				if ((t2 - t1) <= (lease / 14400 + 1)) {
					/* timed out, enter rebinding state */
					state = REBINDING;
					timeout = now + (t2 - t1);
					DEBUG(LOG_INFO, "Entering rebinding state");
				} else {
					/* send a request packet */
					send_renew(xid, server_addr, requested_ip); /* unicast */
					t1 = (t2 - t1) / 2 + t1;
					timeout = t1 + start;
				}
				break;
			case REBINDING:
				/* Either set a new T2, or enter INIT state */
				if ((lease - t2) <= (lease / 14400 + 1)) {
					/* timed out, enter init state */
					state = INIT_SELECTING;
					LOG(LOG_INFO, "Lease lost, entering init state");
#ifdef LED
                    system("/bin/echo i0>/proc/led");
                    if(access("/proc/adsl", F_OK)==0)
                        system("/bin/echo \"0\">/proc/adsl");
#endif /* LED */
					run_script(NULL, "deconfig");
					OldIP = 0;
					timeout = now;
					packet_num = 0;
					change_mode(LISTEN_RAW);
				} else {
					/* send a request packet */
					send_renew(xid, 0, requested_ip); /* broadcast */

					t2 = (lease - t2) / 2 + t2;
					timeout = t2 + start;
				}
				break;
			case RELEASED:
				/* yah, I know, *you* say it would never happen */
				timeout = 0x7fffffff;
#ifdef LED
                system("/bin/echo i0>/proc/led");
                if(access("/proc/adsl", F_OK)==0)
                    system("/bin/echo \"0\">/proc/adsl");
#endif
				break;
			}
		} else if (retval > 0 && listen_mode != LISTEN_NONE && FD_ISSET(fd, &rfds)) {
			/* a packet is ready, read it */

			if (listen_mode == LISTEN_KERNEL)
				len = get_packet(&packet, fd);
			else len = get_raw_packet(&packet, fd);

			if (len == -1 && errno != EINTR) {
				DEBUG(LOG_INFO, "error on read, %s, reopening socket", strerror(errno));
				change_mode(listen_mode); /* just close and reopen */
			}
			if (len < 0) continue;

			if (packet.xid != xid) {
				DEBUG(LOG_INFO, "Ignoring XID %x (our xid is %x)",
					(u_int32_t) packet.xid, xid);
				continue;
			}

			if ((message = get_option(&packet, DHCP_MESSAGE_TYPE)) == NULL) {
				DEBUG(LOG_ERR, "couldnt get option from packet -- ignoring");
				continue;
			}

			switch (state) {
			case INIT_SELECTING:
				/* Must be a DHCPOFFER to one of our xid's */
				if (*message == DHCPOFFER) {
				    FILE *dp;

					if ((temp = get_option(&packet, DHCP_SERVER_ID))) {
						memcpy(&server_addr, temp, 4);
						serv_addr.s_addr=server_addr;
						dp=fopen("/tmp/wan_dhcp_server","w");
					    if(dp!=NULL){
                            fprintf(dp,"%s#\n" ,inet_ntoa(serv_addr));
						    fclose(dp);
                        }
						xid = packet.xid;
						requested_ip = packet.yiaddr;

						/* enter requesting state */
						state = REQUESTING;
						timeout = now;
						packet_num = 0;
					} else {
						DEBUG(LOG_ERR, "No server ID in message");
					}
				}
				break;
			case RENEW_REQUESTED:
			case REQUESTING:
			case RENEWING:
			case REBINDING:
				if (*message == DHCPACK) {
					/* Ron */
					FILE *fp;
					/* Ron */
					if (!(temp = get_option(&packet, DHCP_LEASE_TIME))) {
						LOG(LOG_ERR, "No lease time with ACK, using 1 hour lease");
						lease = 60 * 60;
					} else {
						memcpy(&lease, temp, 4);
						lease = ntohl(lease);
					}

					/* enter bound state */
					t1 = lease / 2;

					/* little fixed point for n * .875 */
					t2 = (lease * 0x7) >> 3;
					temp_addr.s_addr = packet.yiaddr;
					LOG(LOG_INFO, "Lease of %s obtained, lease time %u",
						inet_ntoa(temp_addr), lease);
					/* Ron */
					fp=fopen("/tmp/dhcpc.lease","w");
					if(fp!=NULL){
                                            fprintf(fp,"%s\n%u\n" ,inet_ntoa(temp_addr) ,lease);
						fclose(fp);
					}
					/* Ron */
					start = now;
					timeout = t1 + start;
					requested_ip = packet.yiaddr;
					run_script(&packet,
						   ((state == RENEWING || state == REBINDING) ? "renew" : "bound"));
					OldIP = requested_ip;
					if (!(temp = get_option(&packet, DHCP_ROUTER))) {
						LOG(LOG_ERR, "No router with ACK, using 1 hour lease");
						OldGW = 0;
					} else {
						memcpy(&OldGW, temp, 4);
					}
#ifdef AUTO_IP
					in_auto_ip=0;
#endif
					state = BOUND;
#ifdef LED
                        if(!strcmp(ethwan_enable,"1") && !strcmp(ethwan_encapmod,"pptp"))
                        {
                            system("/bin/echo io>/proc/led");
                            system("/bin/echo io>/proc/led");
                        }
                        else
                        {
                            system("/bin/echo i1>/proc/led");
                            system("/bin/echo i1>/proc/led");
                        }
        if(strcmp(ethwan_enable,"1")){
          if(access("/proc/adsl", F_OK)==0)
               system("/bin/echo \"1\">/proc/adsl");
           }
#endif /* LED */
					change_mode(LISTEN_NONE);
					//background();

				} else if (*message == DHCPNAK) {
					/* return to init state */
					LOG(LOG_INFO, "Received DHCP NAK");
					run_script(&packet, "nak");
					if (state != REQUESTING)
					{
						OldIP = 0;
#ifdef LED
				        system("/bin/echo i0>/proc/led");
                        if(access("/proc/adsl", F_OK)==0)
                            system("/bin/echo \"0\">/proc/adsl");
#endif /* LED */
						run_script(NULL, "deconfig");
					}
					state = INIT_SELECTING;
					timeout = now;
					requested_ip = 0;
					packet_num = 0;
					change_mode(LISTEN_RAW);
					sleep(3); /* avoid excessive network traffic */
				}
				break;
			/* case BOUND, RELEASED: - ignore all packets */
			}
		} else if (retval == -1 && errno == EINTR) {
			/* a signal was caught */

		} else {
			/* An error occured */
			DEBUG(LOG_ERR, "Error on select");
		}

	}
#ifdef LED
        if(!strcmp(ethwan_enable,"1") && !strcmp(ethwan_encapmod,"pptp"))
			system("/bin/echo io>/proc/led");
        else
            system("/bin/echo i1>/proc/led");
        if(strcmp(ethwan_enable,"1")){
  if(access("/proc/adsl", F_OK)==0)
      system("/bin/echo \"1\">/proc/adsl");
    }
#endif
    nvram_safe_free(ethwan_enable);
    nvram_safe_free(ethwan_encapmod);

	return 0;
}

