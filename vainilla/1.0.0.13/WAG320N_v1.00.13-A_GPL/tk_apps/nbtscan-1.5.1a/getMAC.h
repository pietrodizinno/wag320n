/*
 Copyright - 2005 SerComm Corporation.

 This program is free software; you can distribute it and/or modify it
 under the terms of the GNU General Public License (Version 2) as
 published by the Free Software Foundation.

 This program is distributed in the hope it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
*/

#ifndef _GETMAC_H_
#define _GETMAC_H_

#ifdef _WIRELESS_
#define SCAN_IF "br0"
#else
#define SCAN_IF "eth0"
#endif

int getMAC(struct in_addr ip_addr,char mac_addr[32],char *interface);
int ClrMAC(struct in_addr ip_addr,char *interface);

#endif
