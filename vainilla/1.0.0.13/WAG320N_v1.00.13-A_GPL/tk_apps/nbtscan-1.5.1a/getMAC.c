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
#include <stdlib.h>
#include <stdio.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
int getMAC(struct in_addr ip_addr,char *mac_addr,char *interface)
{
  struct sockaddr_in sin = { 0 };
  struct arpreq myarp = { { 0 } };
  int sockfd;
  unsigned char *ptr;
#if 0 
  struct timespec ts;

  ts.tv_sec=0;
  ts.tv_nsec=1;
  
  if(nanosleep(&ts,NULL)<0){
	  return -1;
  }
#endif	  
  sin.sin_family = AF_INET;
  sin.sin_addr=ip_addr;
  
  memcpy(&myarp.arp_pa, &sin, sizeof myarp.arp_pa);         
  strcpy(myarp.arp_dev, interface);
  
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	 return -1;
  }
  
  if (ioctl(sockfd, SIOCGARP, &myarp) == -1) {
  	  close(sockfd); 
	  return -1;
  }
  
  close(sockfd);
  
  if(myarp.arp_flags==0)
	return -1;
  
 

  ptr = (unsigned char *)&myarp.arp_ha.sa_data[0];
  if( *ptr==*(ptr+1) && 
  	  *ptr==*(ptr+2) && 
  	  *ptr==*(ptr+3) && 
  	  *ptr==*(ptr+4) && 
  	  *ptr==*(ptr+5) )
	  return -1;
  
  sprintf(mac_addr,"%02X:%02X:%02X:%02X:%02X:%02X",*ptr, *(ptr+1),*(ptr+2),
	 *(ptr+3),*(ptr+4),*(ptr+5));
  return 1;
}

int ClrMAC(struct in_addr ip_addr,char *interface)
{
  struct sockaddr_in sin = { 0 };
  struct arpreq myarp = { { 0 } };
  int sockfd;
#if 1 
  struct timespec ts;

  ts.tv_sec=0;
  ts.tv_nsec=1;
  
  if(nanosleep(&ts,NULL)<0){
	  return -1;
  }
#endif
  sin.sin_family = AF_INET;
  sin.sin_addr=ip_addr;
  
  memcpy(&myarp.arp_pa, &sin, sizeof myarp.arp_pa);         
  strcpy(myarp.arp_dev, interface);
  
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
	 return -1;
  }
  
  if (ioctl(sockfd, SIOCDARP, &myarp) == -1) {
//	  printf("%s\n",inet_ntoa(ip_addr));
//	  perror("ioctl");
  	  close(sockfd);  
	  return -1;
  }

  close(sockfd); 
  return 1;
}

