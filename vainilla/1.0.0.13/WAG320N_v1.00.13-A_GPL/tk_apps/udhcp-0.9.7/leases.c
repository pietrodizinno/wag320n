/*
 * leases.c -- tools to manage DHCP leases
 * Russ Dill <Russ.Dill@asu.edu> July 2001
 */

#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include "debug.h"
#include "dhcpd.h"
#include "files.h"
#include "options.h"
#include "leases.h"
#include "arpping.h"
#include "socket_tools.h"

unsigned char blank_chaddr[] = {[0 ... 15] = 0};
static	time_t time_bef_NTP=0;/* record the old time,use for currect time after ntp get new time */
int isres_ip(u_int32_t addr);

/* clear every lease out that chaddr OR yiaddr matches and is nonzero */
void clear_lease(u_int8_t *chaddr, u_int32_t yiaddr, int ifid, int serverid)
{
	unsigned int i, j;
	for (j = 0; j < 16 && !chaddr[j]; j++);

	for (i = 0; i < server_config[ifid][serverid].max_leases; i++) {
        if (leases[ifid][serverid][i].prop & LEASE_STATIC) {
            continue;
        }
		if ((j != 16 && !memcmp(leases[ifid][serverid][i].chaddr, chaddr, 16)) ||
		    (yiaddr && leases[ifid][serverid][i].yiaddr == yiaddr)) {
			memset(&(leases[ifid][serverid][i]), 0, sizeof(struct dhcpOfferedAddr));
		}
	}

}

#ifdef RONSCODE
/* add a lease into the table, clearing out any old ones */
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, u_int32_t lease, int ifid,int serverid, char *hostname, u_int8_t if_flag)
#else
struct dhcpOfferedAddr *add_lease(u_int8_t *chaddr, u_int32_t yiaddr, u_int32_t lease, int ifid, int serverid)
#endif
{
	struct dhcpOfferedAddr *oldest;
	struct dhcpOfferedAddr *p = NULL;

	/* clean out any old ones */
	clear_lease(chaddr, yiaddr, ifid, serverid);
	if ((p = find_lease_by_chaddr(chaddr, ifid, serverid))) {
	    /* must meet static lease, keep the static lease */
	    return p;
	}

	oldest = oldest_expired_lease(ifid, serverid);
	if (oldest) {
		memcpy(oldest->chaddr, chaddr, 16);
		oldest->yiaddr = yiaddr;
		oldest->expires = time(0) + lease;
#ifdef RONSCODE
		memcpy(oldest->hostname,hostname,256);
		oldest->if_flag = if_flag;
#endif
	}
	return oldest;
}

/* -- Jeff Sun -- Apr.23.2005 -- Modify for update expire time after ntp get correct time */
/* true if a lease has expired */
int lease_expired(struct dhcpOfferedAddr *lease)
{
    time_t now = time(0);
    int c;
    int k;
    unsigned int i;

    /* init time_bef_NTP */
    if(time_bef_NTP==0) time_bef_NTP=now;

    /* ntp get new time,orig time is in year 2000,new time is after 2005.01.01, do update */
    if( now - time_bef_NTP > ((2005-2000)*365*24*60*60) )
    {
    	for (c = 0; c < no_of_ifaces; c++)
		for(k = 0; k < no_of_servers[c]; k++)
		{
			if (server_config[c][k].active == FALSE)
					continue;

        	for (i = 0; i < server_config[c][k].max_leases; i++)
        		if (leases[c][k][i].yiaddr != 0 && server_config[c][k].remaining)
       				leases[c][k][i].expires += (now-time_bef_NTP);
	    }
    }
    time_bef_NTP=now;

	return (lease->expires < (u_int32_t) now);
}


/* Find the oldest expired lease, NULL if there are no expired leases */
struct dhcpOfferedAddr *oldest_expired_lease(int ifid, int serverid)
{
	struct dhcpOfferedAddr *oldest = NULL;
	u_int32_t oldest_lease = time(0);
	unsigned int i;


	for (i = 0; i < server_config[ifid][serverid].max_leases; i++)
		if (oldest_lease > leases[ifid][serverid][i].expires) {
			oldest_lease = leases[ifid][serverid][i].expires;
			oldest = &(leases[ifid][serverid][i]);
		}
	return oldest;

}


/* Find the first lease that matches chaddr, NULL if no match */
struct dhcpOfferedAddr *find_lease_by_chaddr(u_int8_t *chaddr, int ifid, int serverid)
{
	unsigned int i;

	//LOG(LOG_INFO, "into function find_lease_by_chaddr");
	for (i = 0; i < server_config[ifid][serverid].max_leases; i++)
	{
		if (!memcmp(leases[ifid][serverid][i].chaddr, chaddr, 6)) return &(leases[ifid][serverid][i]);
	}
	return NULL;
}


/* Find the first lease that matches yiaddr, NULL is no match */
struct dhcpOfferedAddr *find_lease_by_yiaddr(u_int32_t yiaddr, int ifid, int serverid)
{
	unsigned int i;

	for (i = 0; i < server_config[ifid][serverid].max_leases; i++)
		if (leases[ifid][serverid][i].yiaddr == yiaddr) return &(leases[ifid][serverid][i]);

	return NULL;
}


/* find an assignable address, it check_expired is true, we check all the expired leases as well.
 * Maybe this should try expired leases by age... */
u_int32_t find_address(int check_expired, int ifid, int serverid)
{
	u_int32_t addr, ret;
	int check_res = 0;

	struct dhcpOfferedAddr *lease = NULL;
	addr = ntohl(server_config[ifid][serverid].start); /* addr is in host order here */
	for (;addr <= ntohl(server_config[ifid][serverid].end); addr++) {
		/* ie, 192.168.55.0 */
		if (!(addr & 0xFF)) continue;

		/* ie, 192.168.55.255 */
		if ((addr & 0xFF) == 0xFF) continue;
		/* Ron */
		/* ie, this ip is server ip */
		if (addr == ntohl(server_config[ifid][serverid].server)) continue;
		/* Ron */
		/* lease is not taken */
        check_res = isres_ip(ntohl(addr));
        if (check_res)
        	continue;
		ret = htonl(addr);
		if ((!(lease = find_lease_by_yiaddr(ret, ifid, serverid)) ||

		     /* or it expired and we are checking for expired leases */
		     (check_expired  && lease_expired(lease))) &&

		     /* and it isn't on the network */
	    	     !check_ip(ret, ifid, serverid)) {
			return ret;
			break;
		}
	}
	return 0;
}

/* check is an IP is taken, if it is, add it to the lease table */
int check_ip(u_int32_t addr, int ifid, int serverid)
{
	struct in_addr temp;
#ifdef RONSCODE
	char hostname[256]="";
	u_int8_t if_flag = ETHERNET_CLIENT;
#endif
    u_char sender_mac[6];

	if (arpping(addr, server_config[ifid][serverid].server, server_config[ifid][serverid].arp, server_config[ifid][0].interface, sender_mac) == 0) {
		temp.s_addr = addr;
	 	LOG(LOG_INFO, "%s belongs to someone, reserving it for %u seconds",
	 		inet_ntoa(temp), server_config[ifid][serverid].conflict_time);
#ifdef RONSCODE
		add_lease(sender_mac, addr, server_config[ifid][serverid].conflict_time, ifid, serverid, hostname, if_flag);
#else
		add_lease(sender_mac, addr, server_config[ifid][serverid].conflict_time, ifid, serverid);
#endif
		return 1;
	} else
	{
		return 0;
	}
}

#include <stdarg.h>
void print2console(char *format,...){
 va_list args;
    FILE *fp;

    fp = fopen("/dev/console","a");
    if(!fp){
        return;
    }
    va_start(args,format);
    vfprintf(fp,format,args);
    va_end(args);
    fprintf(fp,"\n");
    fflush(fp);
    fclose(fp);
   // system("/bin/chmod 777 /var/cgitest");
}

static int My_Pipe(char *command, char **output)
{
    FILE *fp;
    char buf[4096];
    int len=0;

    *output=malloc(1);
    strcpy(*output, "");
    if((fp=popen(command, "r"))==NULL)
        return(-1);
    while((fgets(buf, 4096, fp)) != NULL){
        len=strlen(*output)+strlen(buf);
        if((*output=realloc(*output, (sizeof(char) * (len+1))))==NULL)
            return(-1);
        strcat(*output, buf);
    }
    pclose(fp);
    return len;
}

int find_iface_by_chaddr(u_int8_t *chaddr,u_int8_t *if_flag)
{
	char *wlan_buff;
	int wlanlength = 0;
	char mac[18]="";
		
	// get wireless client mac list
    My_Pipe("/usr/sbin/wlctl assoclist 2>/dev/null",&wlan_buff);
	
	wlanlength = strlen(wlan_buff);

	if(wlanlength == 0)
	{
		free(wlan_buff);
		return 0;
	}
	else
   	{
   		sprintf(mac,"%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX"
   				,chaddr[0]
   				,chaddr[1]
   				,chaddr[2]
   				,chaddr[3]
   				,chaddr[4]
   				,chaddr[5]);
//   		print2console("[%s,%d] wireless dhcp client mac <%s>\n",__FUNCTION__,__LINE__,mac);
   		   		
   		if(strstr(wlan_buff,mac))
   		{
   			*if_flag = WLAN_CLIENT;
  // 			print2console("[%s,%d] Found the wireless dhcp client \n",__FUNCTION__,__LINE__);
   		}
   		else
   			*if_flag = ETHERNET_CLIENT;
   	}
   	
   	free(wlan_buff);
   	
	return 1;
}

int isres_ip(u_int32_t addr)
{
    struct reserve_ip check_ip; 
    char *dhcp_reserved=NULL;
    char *p_reserved1=NULL, *p_reserved2=NULL;
    char p_one_reserved[66];
    
    dhcp_reserved = nvram_get("reservation_static_table");
    if(dhcp_reserved == NULL){
        return 0;
    }
    
    p_reserved1 = dhcp_reserved;
    while(*p_reserved1){
        memset(&check_ip, 0, sizeof(struct reserve_ip));
        memset(p_one_reserved, 0, 66);
        p_reserved2 = strchr(p_reserved1, '\2');
        if(p_reserved2 == NULL){
            free(dhcp_reserved);
            return 0;   
        }
        strncpy(p_one_reserved, p_reserved1, p_reserved2-p_reserved1);
        sscanf(p_one_reserved, "%[^\1]\1%[^\1]\1%s", check_ip.res_devicename, check_ip.res_ip,check_ip.res_mac);
     //   print2console("[%s %d]check_ip.res_ip=%s  %d\n", __FUNCTION__, __LINE__, check_ip.res_ip, addr);
        if(addr == inet_addr(check_ip.res_ip)){
            free(dhcp_reserved);
            return 1;
        }
        p_reserved1 = p_reserved2+1;
    }
    free(dhcp_reserved);
    return 0;
}
