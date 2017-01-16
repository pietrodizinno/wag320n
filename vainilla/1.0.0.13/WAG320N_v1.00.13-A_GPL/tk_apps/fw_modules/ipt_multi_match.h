#ifndef _IPT_MULTI_MATCH_H
#define _IPT_MULTI_MATCH_H

/******************************************************************************************
 *
 *  Copyright - 2005 SerComm Corporation.
 *
 *	All Rights Reserved. 
 *	SerComm Corporation reserves the right to make changes to this document without notice.
 *	SerComm Corporation makes no warranty, representation or guarantee regarding 
 *	the suitability of its products for any particular purpose. SerComm Corporation assumes 
 *	no liability arising out of the application or use of any product or circuit.
 *	SerComm Corporation specifically disclaims any and all liability, 
 *	including without limitation consequential or incidental damages; 
 *	neither does it convey any license under its patent rights, nor the rights of others.
 *
 *****************************************************************************************/

#define MAX_MAC_NUM 8
#define MAX_MAC_LENGTH 6
#define MAX_IP_NUM 6
#define MAX_IP_RANGE_NUM 2

struct ipt_multi_match_info {
	unsigned char MAC[MAX_MAC_NUM][MAX_MAC_LENGTH];
	unsigned int ip_addr[MAX_IP_NUM];
	unsigned int ip_range[MAX_IP_RANGE_NUM][2];
	u_int16_t invert;
};

#endif /* _IPT_MULTI_MATCH_H */
