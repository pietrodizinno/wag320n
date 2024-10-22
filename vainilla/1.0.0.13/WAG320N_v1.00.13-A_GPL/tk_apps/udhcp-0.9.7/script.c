/* script.c
 *
 * Functions to call the DHCP client notification scripts
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

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/sysinfo.h>

#include "options.h"
#include "dhcpd.h"
#include "dhcpc.h"
#include "packet.h"
#include "options.h"
#include "debug.h"
#include "nvram.h"

/* get a rough idea of how long an option will be (rounding up...) */
static int max_option_length[] = {
	[OPTION_IP] =		sizeof("255.255.255.255 "),
	[OPTION_IP_PAIR] =	sizeof("255.255.255.255 ") * 2,
	[OPTION_STRING] =	1,
	[OPTION_BOOLEAN] =	sizeof("yes "),
	[OPTION_U8] =		sizeof("255 "),
	[OPTION_U16] =		sizeof("65535 "),
	[OPTION_S16] =		sizeof("-32768 "),
	[OPTION_U32] =		sizeof("4294967295 "),
	[OPTION_S32] =		sizeof("-2147483684 "),
};

int myPipe(char *command, char **output);

static int upper_length(int length, struct dhcp_option *option)
{
	return max_option_length[option->flags & TYPE_MASK] *
	       (length / option_lengths[option->flags & TYPE_MASK]);
}


static int sprintip(char *dest, char *pre, unsigned char *ip) {
	return sprintf(dest, "%s%d.%d.%d.%d ", pre, ip[0], ip[1], ip[2], ip[3]);
}

extern u_int32_t OldGW;
extern u_int32_t NewGW;

/* Fill dest with the text of option 'option'. */
static void fill_options(char *dest, unsigned char *option, struct dhcp_option *type_p)
{
	int type, optlen;
	u_int16_t val_u16;
	int16_t val_s16;
	u_int32_t val_u32;
	int32_t val_s32;
	int len = option[OPT_LEN - 2];

	dest += sprintf(dest, "%s=", type_p->name);

	type = type_p->flags & TYPE_MASK;
	optlen = option_lengths[type];
	for(;;) {
		switch (type) {
		case OPTION_IP_PAIR:
			dest += sprintip(dest, "", option);
			*(dest++) = '/';
			option += 4;
			optlen = 4;
		case OPTION_IP:	/* Works regardless of host byte order. */
		    if(strcmp(type_p->name,"router")==0){
		        memcpy(&NewGW,option,4);
            }
			dest += sprintip(dest, "", option);
 			break;
		case OPTION_BOOLEAN:
			dest += sprintf(dest, *option ? "yes " : "no ");
			break;
		case OPTION_U8:
			dest += sprintf(dest, "%u ", *option);
			break;
		case OPTION_U16:
			memcpy(&val_u16, option, 2);
			dest += sprintf(dest, "%u ", ntohs(val_u16));
			break;
		case OPTION_S16:
			memcpy(&val_s16, option, 2);
			dest += sprintf(dest, "%d ", ntohs(val_s16));
			break;
		case OPTION_U32:
			memcpy(&val_u32, option, 4);
			dest += sprintf(dest, "%u ", (u_int32_t) ntohl(val_u32));
			break;
		case OPTION_S32:
			memcpy(&val_s32, option, 4);
			dest += sprintf(dest, "%d ", (int32_t) ntohl(val_s32));
			break;
		case OPTION_STRING:
			memcpy(dest, option, len);
			dest[len] = '\0';
			return;	 /* Short circuit this case */
		}
		option += optlen;
		len -= optlen;
		if (len <= 0) break;
	}
}


static char *find_env(const char *prefix, char *defaultstr)
{
	extern char **environ;
	char **ptr;
	const int len = strlen(prefix);

	for (ptr = environ; *ptr != NULL; ptr++) {
		if (strncmp(prefix, *ptr, len) == 0)
			return *ptr;
	}
	return defaultstr;
}

extern u_int32_t OldIP;
/* put all the paramaters into an environment */
static char **fill_envp(struct dhcpMessage *packet)
{
	int num_options = 0;
	int i, j;
	char **envp;
	unsigned char *temp;
	char over = 0;
	char fixed_dns[64]={0}, *pDNS=NULL;
	
	if (packet == NULL)
		num_options = 0;
	else {
		for (i = 0; options[i].code; i++)
			if (get_option(packet, options[i].code))
				num_options++;
		if (packet->siaddr) num_options++;
		if ((temp = get_option(packet, DHCP_OPTION_OVER)))
			over = *temp;
		if (!(over & FILE_FIELD) && packet->file[0]) num_options++;
		if (!(over & SNAME_FIELD) && packet->sname[0]) num_options++;
	}

	envp = malloc((num_options + 12) * sizeof(char *));
	envp[0] = malloc(sizeof("interface=") + strlen(client_config.interface));
	sprintf(envp[0], "interface=%s", client_config.interface);
	envp[1] = find_env("PATH", "PATH=/bin:/usr/bin:/sbin:/usr/sbin");
	envp[2] = find_env("HOME", "HOME=/");

	if (packet == NULL) {
		envp[3] = NULL;
		return envp;
	}

	envp[3] = malloc(sizeof("ip=255.255.255.255"));
	sprintip(envp[3], "ip=", (unsigned char *) &packet->yiaddr);
	for (i = 0, j = 4; options[i].code; i++) {
		if ((temp = get_option(packet, options[i].code))) {
			envp[j] = malloc(upper_length(temp[OPT_LEN - 2], &options[i]) + strlen(options[i].name) + 2);
			fill_options(envp[j], temp, &options[i]);
			j++;
		}
	}
	if (packet->siaddr) {
		envp[j] = malloc(sizeof("siaddr=255.255.255.255"));
		sprintip(envp[j++], "siaddr=", (unsigned char *) &packet->siaddr);
	}
	if (!(over & FILE_FIELD) && packet->file[0]) {
		/* watch out for invalid packets */
		packet->file[sizeof(packet->file) - 1] = '\0';
		envp[j] = malloc(sizeof("boot_file=") + strlen(packet->file));
		sprintf(envp[j++], "boot_file=%s", packet->file);
	}
	if (!(over & SNAME_FIELD) && packet->sname[0]) {
		/* watch out for invalid packets */
		packet->sname[sizeof(packet->sname) - 1] = '\0';
		envp[j] = malloc(sizeof("sname=") + strlen(packet->sname));
		sprintf(envp[j++], "sname=%s", packet->sname);
	}
	//add for indicate wan ip change or not

	if (packet->yiaddr != OldIP) {
		envp[j] = malloc(sizeof("ServiceRestart=1"));
		sprintf(envp[j++], "ServiceRestart=1");
	}
	if (NewGW != OldGW) {
		envp[j] = malloc(sizeof("AddRoute=1"));
		sprintf(envp[j++], "AddRoute=1");
    }

	envp[j++] = find_env("SERVER", "SERVER=");
	

	pDNS= nvram_safe_get("dhcp_dns0");		
	if(strlen(pDNS))
		strcat(fixed_dns, pDNS);
	free(pDNS);
	pDNS=NULL;

	pDNS= nvram_safe_get("dhcp_dns1");		
	if(strlen(pDNS)){
		if(strlen(fixed_dns))
			strcat(fixed_dns, " ");
		strcat(fixed_dns, pDNS);
	}
	free(pDNS);
	pDNS=NULL;
	
	pDNS= nvram_safe_get("dhcp_dns2");		
	if(strlen(pDNS)){
		if(strlen(fixed_dns))
			strcat(fixed_dns, " ");
		strcat(fixed_dns, pDNS);
	}
	free(pDNS);
	pDNS=NULL;
	
	envp[j] = malloc(strlen("fixed_dns=") + strlen(fixed_dns)+1);
	sprintf(envp[j++], "fixed_dns=%s",fixed_dns);
	envp[j] = NULL;
	return envp;
}


int get_sockfd(void)
{

	 int sockfd = -1;

		//if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
		if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		{
			perror("user: socket creating failed");
			return (-1);
		}
	return sockfd;
}

#ifdef AUTO_IP
void config_auto_ip(char *ip_addr, char *netmask)
{
	char *msg[3]={NULL},ip_str[24], mask_str[32];
	char cmd_str[128]={0};
	
	sprintf(ip_str, "ip=%s",ip_addr);
	sprintf(mask_str, "subnet=%s", netmask);
	msg[0]=ip_str;
	msg[1]=mask_str;
	msg[2]=NULL;
	//run_bound(msg);
	sprintf(cmd_str, "/sbin/ifconfig br0 %s netmask %s 2>/dev/null", ip_addr, netmask);
	system(cmd_str);
	system("/sbin/route add -net default br0 2>/dev/null"); 
	system("/sbin/route add -net 239.0.0.0 netmask 255.0.0.0 br0 2>/dev/null");
	system("/etc/init.d/rc.samba 2>/dev/null"); 
}
#endif
/* Call a script with a par file and env vars */
void run_script(struct dhcpMessage *packet, const char *name)
{
	int pid;
	char **envp;

	if (client_config.script == NULL)
		return;

	// add by john.  If deconfig, eth1 will lost ip, default gw will lost at sametime
	if (strcmp(name,"deconfig")==0)
		OldGW=0;

	/* call script */
	pid = fork();
	if (pid) {
		waitpid(pid, NULL, 0);
		return;
	} else if (pid == 0) {
		envp = fill_envp(packet);

		/* close fd's? */

		/* exec script */
		DEBUG(LOG_INFO, "execle'ing %s", client_config.script);
		execle(client_config.script, client_config.script,
		       name, NULL, envp);
		LOG(LOG_ERR, "script %s failed: %s",
		    client_config.script, strerror(errno));
		exit(1);
	}
}
