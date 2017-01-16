#ifndef _SERVERPACKET_H
#define _SERVERPACKET_H


int sendOffer(struct dhcpMessage *oldpacket, int ifid, int serverid);
int sendNAK(struct dhcpMessage *oldpacket, int ifid,  int serverid);
int sendACK(struct dhcpMessage *oldpacket, u_int32_t yiaddr, int ifid,  int serverid);
int send_inform(struct dhcpMessage *oldpacket, int ifid, int serverid);


#endif
