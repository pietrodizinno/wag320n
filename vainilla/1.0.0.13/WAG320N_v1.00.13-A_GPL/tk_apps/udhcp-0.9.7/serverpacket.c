/* serverpacket.c
 *
 * Constuct and send DHCP server packets
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "packet.h"
#include "debug.h"
#include "dhcpd.h"
#include "options.h"
#include "leases.h"
#include "socket_tools.h"
struct reserve_ip check;
int isres_ip(u_int32_t addr);
int isreserve_mac(u_int8_t *chaddr);
int mcpy(unsigned char *s);

/* send a packet to giaddr using the kernel ip stack */
static int send_packet_to_relay(struct dhcpMessage *payload, int ifid, int serverid)
{
	DEBUG(LOG_INFO, "Forwarding packet to relay");

	return kernel_packet(payload, server_config[ifid][serverid].server, SERVER_PORT,
			payload->giaddr, SERVER_PORT);
}


/* send a packet to a specific arp address and ip address by creating our own ip packet */
static int send_packet_to_client(struct dhcpMessage *payload, int force_broadcast, int ifid, int serverid)
{
	unsigned char *chaddr;
	u_int32_t ciaddr;

	if (force_broadcast) {
		DEBUG(LOG_INFO, "broadcasting packet to client (NAK)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	} else if (payload->ciaddr) {
		DEBUG(LOG_INFO, "unicasting packet to client ciaddr");
		ciaddr = payload->ciaddr;
		chaddr = payload->chaddr;
	} else if (ntohs(payload->flags) & BROADCAST_FLAG) {
		DEBUG(LOG_INFO, "broadcasting packet to client (requested)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	} else {
		DEBUG(LOG_INFO, "unicasting packet to client yiaddr");
		ciaddr = payload->yiaddr;
		chaddr = payload->chaddr;
	}
	return raw_packet(payload, server_config[ifid][serverid].server, SERVER_PORT,
			ciaddr, CLIENT_PORT, chaddr, server_config[ifid][serverid].ifindex);
}


/* send a dhcp packet, if force broadcast is set, the packet will be broadcast to the client */
static int send_packet(struct dhcpMessage *payload, int force_broadcast, int ifid, int serverid)
{
	int ret;

	if (payload->giaddr)
		ret = send_packet_to_relay(payload, ifid, serverid);
	else ret = send_packet_to_client(payload, force_broadcast, ifid, serverid);
	return ret;
}


static void init_packet(struct dhcpMessage *packet, struct dhcpMessage *oldpacket, char type, int ifid, int serverid)
{
	init_header(packet, type);
	packet->xid = oldpacket->xid;
	memcpy(packet->chaddr, oldpacket->chaddr, 16);
	packet->flags = oldpacket->flags;
	packet->giaddr = oldpacket->giaddr;
	packet->ciaddr = oldpacket->ciaddr;
	add_simple_option(packet->options, DHCP_SERVER_ID, server_config[ifid][serverid].server);
}


/* add in the bootp options */
static void add_bootp_options(struct dhcpMessage *packet, int ifid, int serverid)
{
	packet->siaddr = server_config[ifid][serverid].siaddr;
	if (server_config[ifid][serverid].sname)
		strncpy(packet->sname, server_config[ifid][serverid].sname, sizeof(packet->sname) - 1);
	if (server_config[ifid][serverid].boot_file)
		strncpy(packet->file, server_config[ifid][serverid].boot_file, sizeof(packet->file) - 1);
}


/* send a DHCP OFFER to a DHCP DISCOVER */
int sendOffer(struct dhcpMessage *oldpacket, int ifid, int serverid)
{
	struct dhcpMessage packet;
	struct dhcpOfferedAddr *lease = NULL;
	u_int32_t req_align, lease_time_align = server_config[ifid][serverid].lease;
	unsigned char *req, *lease_time;
	struct option_set *curr;
	struct in_addr addr;
    int check_res = 0;
#ifdef RONSCODE
	char hostname[256];
	u_int8_t if_flag = ETHERNET_CLIENT;
#endif

	init_packet(&packet, oldpacket, DHCPOFFER, ifid, serverid);
    check_res = isreserve_mac(packet.chaddr);
	/* ADDME: if static, short circuit */
	/* the client is in our lease/offered table */
	/* Ron  add 3 lines */
	if((req = get_option(oldpacket, DHCP_REQUESTED_IP)))
		memcpy(&req_align, req, 4);

	//if ((lease = find_lease_by_chaddr(oldpacket->chaddr, ifid))) {
	if(check_res == 1 &&(ntohl(inet_addr(check.res_ip))!=ntohl(server_config[ifid][serverid].server)))
	{
		packet.yiaddr = inet_addr(check.res_ip);
		//memcpy(&packet.yiaddr,&check.res_ip,4);
		//packet.yiaddr = check->res_ip;
	}
	else
	{
		if ((lease = find_lease_by_chaddr(oldpacket->chaddr, ifid, serverid))&& (ntohl(req_align)!=ntohl(server_config[ifid][serverid].server)))
		{
			if (!lease_expired(lease))
				lease_time_align = lease->expires - time(0);
			packet.yiaddr = lease->yiaddr;
			/* Or the client has a requested ip */
		}
		else if ((req = get_option(oldpacket, DHCP_REQUESTED_IP)) &&
				/* Don't look here (ugly hackish thing to do) */
				memcpy(&req_align, req, 4) &&
				(!isres_ip(req_align))&&
				/* and the ip is in the lease range */
				ntohl(req_align) >= ntohl(server_config[ifid][serverid].start) &&
				ntohl(req_align) <= ntohl(server_config[ifid][serverid].end) &&
				/* and its not already taken/offered */ /* ADDME: check that its not a static lease */
				((!(lease = find_lease_by_yiaddr(req_align, ifid, serverid)) ||
				/* or its taken, but expired */ /* ADDME: or maybe in here */
				lease_expired(lease)))) 
		{
		   /* check id addr is not taken by a static ip */
#ifdef RONSCODE
		   if(!check_ip(req_align, ifid, serverid) && (ntohl(req_align)!=ntohl(server_config[ifid][serverid].server)) )
#else
		   if(!check_ip(req_align, ifid, serverid))
#endif
				packet.yiaddr = req_align; /* FIXME: oh my, is there a host using this IP? */
		   else {
			   packet.yiaddr = find_address(0, ifid, serverid);
			   /* try for an expired lease */
			   if (!packet.yiaddr) packet.yiaddr = find_address(1, ifid, serverid);

			}


		/* otherwise, find a free IP */ /*ADDME: is it a static lease? */
		} 
		else 
		{
			packet.yiaddr = find_address(0, ifid, serverid);

			/* try for an expired lease */
			if (!packet.yiaddr) packet.yiaddr = find_address(1, ifid, serverid);
		}
	}

	if(!packet.yiaddr) {
		LOG(LOG_WARNING, "no IP addresses to give -- OFFER abandoned");
		return -1;
	}
#ifdef RONSCODE
    {
		unsigned char *tmp=get_option(oldpacket,DHCP_HOST_NAME);
		int len=0;
		if(tmp){
			len=*(tmp-1);
			len = len > 255?255:len;
			memset(hostname,0,sizeof(hostname));
			strncpy(hostname, (char *)tmp,len);
		}
	}
	
	{
		find_iface_by_chaddr(packet.chaddr,&if_flag);
	}

	if (!add_lease(packet.chaddr, packet.yiaddr, server_config[ifid][serverid].offer_time, ifid, serverid, hostname, if_flag)) {
		LOG(LOG_WARNING, "lease pool is full -- OFFER abandoned");
		return -1;
	}
#else
	if (!add_lease(packet.chaddr, packet.yiaddr, server_config[ifid][serverid].offer_time, ifid, serverid)) {
		LOG(LOG_WARNING, "lease pool is full -- OFFER abandoned");
		return -1;
	}
#endif
	if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) {
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);
		if (lease_time_align > server_config[ifid][serverid].lease)
			lease_time_align = server_config[ifid][serverid].lease;
	}

	/* Make sure we aren't just using the lease time from the previous offer */
	if (lease_time_align < server_config[ifid][serverid].min_lease)
		lease_time_align = server_config[ifid][serverid].lease;
	/* ADDME: end of short circuit */
	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));

	curr = server_config[ifid][serverid].options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet, ifid, serverid);

	addr.s_addr = packet.yiaddr;
	LOG(LOG_INFO, "sending OFFER of %s", inet_ntoa(addr));
	return send_packet(&packet, 0, ifid, serverid);
}


int sendNAK(struct dhcpMessage *oldpacket, int ifid, int serverid)
{
	struct dhcpMessage packet;

	init_packet(&packet, oldpacket, DHCPNAK, ifid, serverid);

	DEBUG(LOG_INFO, "sending NAK");
	return send_packet(&packet, 1, ifid, serverid);
}


int sendACK(struct dhcpMessage *oldpacket, u_int32_t yiaddr, int ifid, int serverid)
{
	struct dhcpMessage packet;
	struct option_set *curr;
	unsigned char *lease_time;
	u_int32_t lease_time_align = server_config[ifid][serverid].lease;
	struct in_addr addr;
#ifdef RONSCODE
	char hostname[256];
	u_int8_t if_flag = ETHERNET_CLIENT;
#endif

#ifdef RONSCODE
    memset(hostname, 0, sizeof(hostname));
#endif
	init_packet(&packet, oldpacket, DHCPACK, ifid, serverid);
	packet.yiaddr = yiaddr;

	if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) {
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);
		if (lease_time_align > server_config[ifid][serverid].lease)
			lease_time_align = server_config[ifid][serverid].lease;
		else if (lease_time_align < server_config[ifid][serverid].min_lease)
			lease_time_align = server_config[ifid][serverid].lease;
	}

	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));

	curr = server_config[ifid][serverid].options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet, ifid, serverid);

	addr.s_addr = packet.yiaddr;
	LOG(LOG_INFO, "sending ACK to %s", inet_ntoa(addr));

	if (send_packet(&packet, 0, ifid, serverid) < 0)
		return -1;
#ifdef RONSCODE
	{
		unsigned char *tmp=get_option(oldpacket,DHCP_HOST_NAME);
		int len=0;
		if(tmp){
			len=*(tmp-1);
			strncpy(hostname, (char *)tmp,len);
		}
	}
	
	{
		find_iface_by_chaddr(packet.chaddr,&if_flag);
	}

	add_lease(packet.chaddr, packet.yiaddr, lease_time_align, ifid, serverid, hostname, if_flag);
#else
	add_lease(packet.chaddr, packet.yiaddr, lease_time_align, ifid, serverid);
#endif
	return 0;
}


int send_inform(struct dhcpMessage *oldpacket, int ifid, int serverid)
{
	struct dhcpMessage packet;
	struct option_set *curr;

	init_packet(&packet, oldpacket, DHCPACK, ifid, serverid);

	curr = server_config[ifid][serverid].options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet, ifid, serverid);

	return send_packet(&packet, 0, ifid, serverid);
}

int isreserve_mac(u_int8_t *chaddr)
{
    char *dhcp_reserved;    
    char *p_reserved1, *p_reserved2,*p_reserved3;
    char p_one_reserved[66];
    unsigned char temp[6][6];
	unsigned char chadd[16];
	int i;

    dhcp_reserved = nvram_get("reservation_static_table");
    //print2console("[%s %d]dhcp_reserved=%s\n", __FUNCTION__, __LINE__, dhcp_reserved);

    if(dhcp_reserved == NULL){
      // print2console("[%s %d]return for dhcp_reserved is empty\n", __FUNCTION__, __LINE__);
        return 0;
    }
    
    p_reserved1=dhcp_reserved;
    while(*p_reserved1){
        memset(p_one_reserved, 0, 66);
        p_reserved2 = strchr(p_reserved1, '\2');
        if(p_reserved2 == NULL){
            free(dhcp_reserved);
            return 0;   
        }

        strncpy(p_one_reserved, p_reserved1, p_reserved2-p_reserved1);
		//print2console("\np_one_reserved=%s\n",p_one_reserved);
        sscanf(p_one_reserved, "%[^\1]\1%[^\1]\1%s", check.res_devicename,check.res_ip,check.res_mac);
		if(!strlen(check.res_devicename)) {
			p_reserved3=strchr(p_one_reserved,'\1');
			p_reserved3=p_reserved3+1;
			sscanf(p_reserved3,"%[^\1]\1%s",check.res_ip,check.res_mac);
		}
        sscanf(check.res_mac,"%[^:]:%[^:]:%[^:]:%[^:]:%[^:]:%[^:]",temp[0],temp[1],temp[2],temp[3],temp[4],temp[5]);
        //print2console("[%s %d]res_mac=%s\n", __FUNCTION__, __LINE__, check.res_mac);
		//print2console("\ncheck.res_devicename=%s\n",check.res_devicename);
	//	print2console("\ncheck.res_ip=%s\n",check.res_ip);
        for(i = 0;i < 6;i++){
            chadd[i] = mcpy(temp[i]);
		}
        if(memcmp(chaddr,chadd,6) == 0){
            free(dhcp_reserved);
            return 1;
        }
        p_reserved1 = p_reserved2+1;
    }
    free(dhcp_reserved);
    return 0;
}

/* joerica_add */
int mcpy(unsigned char *s)
{
	int len;
	int i;
	int dec[5] = {0};
	int hex;
	//char tmp[5] = "";
	len = strlen(s);
	//printf("len = %d\n",len);
	for(i = 0;i < len;i++)
	{
		switch(*s)
		{
			case '0':
				dec[i] = *s++ - 48;
				break;
			case '1':
				dec[i] = *s++ - 48;
				break;
			case '2':
				dec[i] = *s++ - 48;
				break;
			case '3':
				dec[i] = *s++ - 48;
				break;
			case '4':
				dec[i] = *s++ - 48;
				break;
			case '5':
				dec[i] = *s++ - 48;
				break;
			case '6':
				dec[i] = *s++ - 48;
				break;
			case '7':
				dec[i] = *s++ - 48;
				break;
			case '8':
				dec[i] = *s++ - 48;
				break;
			case '9':
				dec[i] = *s++ - 48;
				break;
				
			case 'a':
				dec[i] = *s++ - 87;
				break;	
			case 'b':
				dec[i] = *s++ - 87;
				break;
			case 'c':
				dec[i] = *s++ - 87;
				break;
			case 'd':
				dec[i] = *s++ - 87;
				break;
			case 'e':
				dec[i] = *s++ - 87;
				break;
			case 'f':
				dec[i] = *s++ - 87;
				break;
			case 'A':
				dec[i] = *s++ - 55;
				break;
			case 'B':
				dec[i] = *s++ - 55;
				break;
			case 'C':
				dec[i] = *s++ - 55;
				break;
			case 'D':
				dec[i] = *s++ - 55;
				break;
			case 'E':
				dec[i] = *s++ - 55;
				break;
			case 'F':
				dec[i] = *s++ - 55;
				break;
			default:
				s++;
				break;
		}
		//printf("dec[%d] = %d\n",i,dec[i]);
	}
	hex = dec[0]*16 + dec[1];
	return hex; 
}



