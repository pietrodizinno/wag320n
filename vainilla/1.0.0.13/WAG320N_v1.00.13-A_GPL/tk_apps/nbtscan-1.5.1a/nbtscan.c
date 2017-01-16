#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#if HAVE_STDINT_H
#include <stdint.h>
#endif
#include "statusq.h"
#include "getMAC.h"
#include "range.h"
#include "list.h"
#include "errors.h"
#include "time.h"

typedef struct{
      char ipaddr[16];
      char pc_name[16];
      char macaddr[18];
} attach_info;

static attach_info ad_info[255];
static int thread_done=0;
int quiet=0;


int set_range(char* range_str, struct ip_range* range_struct) {
  if(is_ip(range_str, range_struct)) return 1;
  if(is_range1(range_str, range_struct)) return 1;
  if(is_range2(range_str, range_struct)) return 1;
  return 0;
};

int print_hostinfo(struct in_addr addr, struct nb_host_info* hostinfo, char* sf) {
  int i;
  unsigned char service; /* 16th byte of NetBIOS name */
  char comp_name[16], user_name[16];
  int is_server=0;
  int unique;
  int first_name=1;
  int haha;
  strncpy(comp_name,"UNKNOWN",15);
  strncpy(user_name,"UNKNOWN",15);
  if(hostinfo->header && hostinfo->names) {
    for(i=0; i< hostinfo->header->number_of_names; i++) {
      service = hostinfo->names[i].ascii_name[15];
      unique = ! (hostinfo->names[i].rr_flags & 0x0080);
      if(service == 0  && unique && first_name) {
				/* Unique name, workstation service - this is computer name */ 
	strncpy(comp_name, hostinfo->names[i].ascii_name, 15);
	comp_name[15] = 0;
	first_name = 0;
      };
      if(service == 0x20 && unique) {
	is_server=1;
      }
      if(service == 0x03 && unique) {
	strncpy(user_name, hostinfo->names[i].ascii_name, 15);
	user_name[15]=0;
      };
    };
  };
  
  /* Ron */
  if(strcmp(comp_name,"UNKNOWN")==0){
	strncpy(comp_name,user_name,15);
  }
  
   haha=(htonl(addr.s_addr)& 0x000000FF);
//   printf("haha=%d\n",haha);
   if(strncmp(comp_name,"IS~",3)==0)
		strcpy(ad_info[haha].pc_name,strtok(comp_name+3," "));
   else
		strcpy(ad_info[haha].pc_name,strtok(comp_name," "));
   
   if(strcmp(ad_info[haha].macaddr,"00:00:00:00:00:00")==0)
   sprintf(ad_info[haha].macaddr,"%02X:%02X:%02X:%02X:%02X:%02X\n",
	   hostinfo->footer->adapter_address[0], hostinfo->footer->adapter_address[1],hostinfo->footer->adapter_address[2], hostinfo->footer->adapter_address[3],hostinfo->footer->adapter_address[4], hostinfo->footer->adapter_address[5]);	

  return 1;
};

	
#define BUFFSIZE 1024

void* nbtscan_fun(void *iprange) {
  int timeout=1000, use137=0, bandwidth=0, send_ok=0 ;
  extern char *optarg;
  extern int optind;
  char* target_string;
  char* sf=NULL;
  char* filename =NULL;
  struct ip_range range;
  void *buff;
  int sock, addr_size;
  struct sockaddr_in src_sockaddr, dest_sockaddr;
  struct  in_addr *prev_in_addr=NULL;
  struct  in_addr *next_in_addr;
  struct timeval select_timeout, last_send_time, current_time, diff_time, send_interval;
  struct timeval transmit_started, recv_time;
  struct nb_host_info* hostinfo;
  fd_set* fdsr;
  fd_set* fdsw;
  int size;
  struct list* scanned;
  my_uint32_t rtt_base; /* Base time (seconds) for round trip time calculations */
  float rtt; /* most recent measured RTT, seconds */
  float srtt=0; /* smoothed rtt estimator, seconds */
  float rttvar=0.75; /* smoothed mean deviation, seconds */ 
  double delta; /* used in retransmit timeout calculations */
  int retransmits=1, more_to_send=1, i;
  char errmsg[80];
  char str[80];
  FILE* targetlist=NULL;
  /* Parse supplied options */
  /**************************/
 //	printf("iprange==%s\n",iprange); 
	if((target_string=strdup((char *)iprange))==NULL) 
		err_die("Malloc failed.\n", quiet);
    
	if(!set_range(target_string, &range)) {
		printf("Error: %s is not an IP address or address range.\n", target_string);
		free(target_string);
		exit(0);
	};

  /* Finished with options */
  /*************************/

  /* Prepare socket and address structures */
  /*****************************************/
  sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock < 0) 
    err_die("Failed to create socket", quiet);

  bzero((void*)&src_sockaddr, sizeof(src_sockaddr));
  src_sockaddr.sin_family = AF_INET;
  if(use137) src_sockaddr.sin_port = htons(NB_DGRAM);
  if (bind(sock, (struct sockaddr *)&src_sockaddr, sizeof(src_sockaddr)) == -1) 
    err_die("Failed to bind", quiet);
        
  fdsr=malloc(sizeof(fd_set));
  if(!fdsr)  err_die("Malloc failed", quiet);
  FD_ZERO(fdsr);
  FD_SET(sock, fdsr);
        
  fdsw=malloc(sizeof(fd_set));
  if(!fdsw) err_die("Malloc failed", quiet);
  FD_ZERO(fdsw);
  FD_SET(sock, fdsw);

  /* timeout is in milliseconds */
  select_timeout.tv_sec = timeout / 1000;
  select_timeout.tv_usec = (timeout % 1000) * 1000; /* Microseconds */

  addr_size = sizeof(struct sockaddr_in);

  next_in_addr = malloc(sizeof(struct  in_addr));
  if(!next_in_addr) err_die("Malloc failed", quiet);

  buff=malloc(BUFFSIZE);
  if(!buff) err_die("Malloc failed", quiet);
	
  /* Calculate interval between subsequent sends */

  timerclear(&send_interval);
  if(bandwidth) send_interval.tv_usec = 
		  (NBNAME_REQUEST_SIZE + UDP_HEADER_SIZE + IP_HEADER_SIZE)*8*1000000 /
		  bandwidth;  /* Send interval in microseconds */
  else /* Assuming 10baseT bandwidth */
    send_interval.tv_usec = 1; /* for 10baseT interval should be about 1 ms */
  if (send_interval.tv_usec >= 1000000) {
    send_interval.tv_sec = send_interval.tv_usec / 1000000;
    send_interval.tv_usec = send_interval.tv_usec % 1000000;
  }
	
  gettimeofday(&last_send_time, NULL); /* Get current time */

  rtt_base = last_send_time.tv_sec; 

  /* Send queries, receive answers and print results */
  /***************************************************/
	
  scanned = new_list();

	
  for(i=0; i <= retransmits; i++) {
    gettimeofday(&transmit_started, NULL);
    while ( (select(sock+1, fdsr, fdsw, NULL, &select_timeout)) > 0) {
      if(FD_ISSET(sock, fdsr)) {
	if ( (size = recvfrom(sock, buff, BUFFSIZE, 0,
			      (struct sockaddr*)&dest_sockaddr, (socklen_t *)&addr_size)) <= 0 ) {
	  snprintf(errmsg, 80, "%s\tRecvfrom failed", inet_ntoa(dest_sockaddr.sin_addr));
	  err_print(errmsg, quiet);
	  continue;
	};
	gettimeofday(&recv_time, NULL);
	hostinfo = (struct nb_host_info*)parse_response(buff, size);
	if(!hostinfo) {
	  err_print("parse_response returned NULL", quiet);
	  continue;
	};

				/* If this packet isn't a duplicate */
	if(insert(scanned, ntohl(dest_sockaddr.sin_addr.s_addr))) {
	  rtt = recv_time.tv_sec + 
	    recv_time.tv_usec/1000000 - rtt_base - 
	    hostinfo->header->transaction_id/1000;
	  /* Using algorithm described in Stevens' 
	     Unix Network Programming */
	  delta = rtt - srtt;
	  srtt += delta / 8;
	  if(delta < 0.0) delta = - delta;
	  rttvar += (delta - rttvar) / 4 ;
				
	    print_hostinfo(dest_sockaddr.sin_addr, hostinfo,sf);
	};
	free(hostinfo);
      };

      FD_ZERO(fdsr);
      FD_SET(sock, fdsr);		

      /* check if send_interval time passed since last send */
      gettimeofday(&current_time, NULL);
      timersub(&current_time, &last_send_time, &diff_time);
      send_ok = timercmp(&diff_time, &send_interval, >=);
			
		
      if(more_to_send && FD_ISSET(sock, fdsw) && send_ok) {
	if(targetlist) {
	  if(fgets(str, 80, targetlist)) {
	    if(!inet_aton(str, next_in_addr)) {
            /* if(!inet_pton(AF_INET, str, next_in_addr)) { */
	      fprintf(stderr,"%s - bad IP address\n", str);
	    } else {
	      if(!in_list(scanned, ntohl(next_in_addr->s_addr))) 
	        send_query(sock, *next_in_addr, rtt_base);
	      
	    }
	  } else {
	    if(feof(targetlist)) {
	      more_to_send=0; 
	      FD_ZERO(fdsw);
              /* timeout is in milliseconds */
	      select_timeout.tv_sec = timeout / 1000;
              select_timeout.tv_usec = (timeout % 1000) * 1000; /* Microseconds */
	      continue;
	    } else {
	      snprintf(errmsg, 80, "Read failed from file %s", filename);
	      err_die(errmsg, quiet);
	    }
	  }
	} else if(next_address(&range, prev_in_addr, next_in_addr) ) {
	  if(!in_list(scanned, ntohl(next_in_addr->s_addr))){ 
            char mac[18];
	    int haha=0;
	    // boone,clear 1 item 1 time in arp cache,after clear flags we can get fresh status.
	    ClrMAC(*next_in_addr,SCAN_IF); 
	    send_query(sock, *next_in_addr, rtt_base);
	    //printf("%s\n",inet_ntoa(*next_in_addr));
	    if(getMAC(*next_in_addr,mac,SCAN_IF)>0){
		    if(strcmp(mac,"00:00:00:00:00:00")!=0 && strcmp(mac,"FF:FF:FF:FF:FF:FF")!=0){
			    haha=(htonl(next_in_addr->s_addr)& 0x000000FF);
			    strcpy(ad_info[haha].ipaddr,inet_ntoa(*next_in_addr));
		//	    printf("%s haha==%d\n",inet_ntoa(*next_in_addr),haha);
		    	    strcpy(ad_info[haha].macaddr,mac);
			    strcpy(ad_info[haha].pc_name,"UNKNOWN");		   
			    //ad_num++;
		    }
	    }

	  }
	  
	  prev_in_addr=next_in_addr;
	  /* Update last send time */
	  gettimeofday(&last_send_time, NULL); 
	} else { /* No more queries to send */
	  more_to_send=0; 
	  FD_ZERO(fdsw);
          /* timeout is in milliseconds */
          select_timeout.tv_sec = timeout / 1000;
          select_timeout.tv_usec = (timeout % 1000) * 1000; /* Microseconds */
	  continue;
	};
      };	
      if(more_to_send) {
	FD_ZERO(fdsw);
	FD_SET(sock, fdsw);
      };
    };

    if (i>=retransmits) break; /* If we are not going to retransmit
				 we can finish right now without waiting */
#if 0 
    rto = (srtt + 4 * rttvar) * (i+1);

    if ( rto < 2.0 ) rto = 2.0;
    if ( rto > 60.0 ) rto = 60.0;
    gettimeofday(&now, NULL);
		
    if(now.tv_sec < (transmit_started.tv_sec+rto)) 
      sleep((transmit_started.tv_sec+rto)-now.tv_sec);
    prev_in_addr = NULL ;
    more_to_send=1;
    FD_ZERO(fdsw);
    FD_SET(sock, fdsw);
    FD_ZERO(fdsr);
    FD_SET(sock, fdsr);
#endif
  };

  delete_list(scanned);
  thread_done++;
  return NULL;
};

//=============================================================
// The funciton myPipe will put the result of command into ouput
//=============================================================
int myPipe(char *command, char **output)
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

int main(int argc,char **argv)
{
	int i=0;
	char *pt=argv[1];
	char *pt2;
	char tmp[5][128];
	FILE *fp;
	pthread_t p1,p2,p3,p4,p5;
	// boone,add for parse arp cache finally
    char *arpbuff,*pt1;
    char ipbuf[16],hatype[8],flags[8],hwbuf[18],devname[8];
    int pos;
    struct in_addr arpip;
	
	memset(ad_info,0,sizeof(attach_info)*255);
	
	if((pt2=strrchr(pt,'.'))==NULL){
		return 0;
	}
	*pt2='\0';

	sprintf(tmp[0],"%s.1-50",pt);
	pthread_create(&p1,NULL,nbtscan_fun,tmp[0]);
	sprintf(tmp[1],"%s.51-100",pt);
	pthread_create(&p2,NULL,nbtscan_fun,tmp[1]);	
	sprintf(tmp[2],"%s.101-150",pt);
	pthread_create(&p3,NULL,nbtscan_fun,tmp[2]);	
	sprintf(tmp[3],"%s.151-200",pt);
	pthread_create(&p4,NULL,nbtscan_fun,tmp[3]);	
	sprintf(tmp[4],"%s.201-254",pt);
	pthread_create(&p5,NULL,nbtscan_fun,tmp[4]);	
		
	while(thread_done<5){
		sleep(1);
	}
	
    /* 
     *boone,after clrmac one by one,we can't get flags of arp cache immediately above,
     *so we check arp cache here.
     */
    sleep(1);
    myPipe("/bin/cat /proc/net/arp",&arpbuff);
    pt1 = arpbuff;
	for(pt1=strtok(pt1,"\n");pt1;pt1=strtok(NULL,"\n"))
	{
	    if(strstr(pt1,"IP address")) continue;
        
		sscanf(pt1, "%s %s %s %s * %s",
		   ipbuf, hatype, flags, hwbuf, devname);
	    
	    if(strtol(flags,NULL,16)==0x2)
	    {
	        if(inet_aton(ipbuf,&arpip) && !strcmp(devname,SCAN_IF))
	        {
	            pos=(htonl(arpip.s_addr)& 0x000000FF);
	            strcpy(ad_info[pos].ipaddr,ipbuf);
	            strcpy(ad_info[pos].macaddr,hwbuf);
	        }
	    }
    }
    free(arpbuff);
	
	fp=fopen("/tmp/nbtscan.out","w");
	if(fp==NULL)
		return 0;
	for(i=0;i<255;i++){
		if(ad_info[i].ipaddr[0]!='\0'){
		    // boone:sometimes there's not enough time to get the host names
		    // especially when client is wireless.
		    if(ad_info[i].pc_name[0]=='\0')
		        strcpy(ad_info[i].pc_name,"UNKNOWN");
			fprintf(fp,"%s;%s;%s\n",ad_info[i].ipaddr,ad_info[i].pc_name,ad_info[i].macaddr);
			printf("%s;%s;%s\n",ad_info[i].ipaddr,ad_info[i].pc_name,ad_info[i].macaddr);
		}
  	}
	fclose(fp);
	return 0;
}
