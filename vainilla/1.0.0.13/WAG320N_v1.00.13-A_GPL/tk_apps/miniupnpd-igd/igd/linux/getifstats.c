/* $Id: getifstats.c,v 1.5 2009-06-16 06:31:12 shearer_lu Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <unistd.h>

#include "bcmnet.h"
#include "getifstats.h"
#include "nvram.h"

extern char ext_if_name[];

#define	DSL_STATUS_FILE		"/tmp/dsl_status"

static int get_ethernet_link(void)
{
    int s;
    struct ifreq ifr;
    int speed = 100;
    
    s = socket(PF_INET, SOCK_DGRAM, 0);
    if(s < 0)
    	return speed;       
    strcpy(ifr.ifr_name, "eth0");
    ifr.ifr_data = (void *)&speed;
    if (ioctl(s, SIOCGETHWANPORTLINKSTATUS, &ifr) < 0) {
        close(s);
        return speed;
    }
    close(s);
    return speed;
}

int get_adsl_rate(unsigned long *downrate_p,unsigned long *uprate_p)
{
    char buf[256]={0};
    char *p0=NULL, *p1=NULL;
    FILE *fp=NULL;

    /* ----- get firmware ------ */
    if((fp=fopen(DSL_STATUS_FILE,"r"))==NULL)
        return -1;
 	while(fgets(buf, sizeof(buf)-1, fp)){
 		if(strstr(buf, "Path:") && strstr(buf, "Upstream rate")){
 			//Path:   0, Upstream rate = 890 Kbps, Downstream rate = 7904 Kbps
 			p0=strchr(buf, '=');
 			if(!p0)
 				continue;
 			*p0++;
 			p1=strstr(p0, "Kbps");
 			if(!p1)
 				continue;
 			*p1=0;
 			*uprate_p=strtoul(p0, NULL, 10);
 			*uprate_p*=1000;
 			p0=strchr(p1+1, '=');
 			if(!p0)
 				continue;
 			*p0++;
 			p1=strstr(p0, "Kbps");
 			if(!p1)
 				continue;
 			*p1=0;
 			*downrate_p=strtoul(p0, NULL, 10);
 			*downrate_p*=1000;
 		}
 	}
	fclose(fp);
    return 0;
}

int get_wan_up(char *ifname);

int getifstats(const char * ifname, struct ifdata * data, int link_speed)
{
	FILE *f;
	char line[512]={0};
	char * p=NULL;
	int i;
	int r = -1;
	char *ethwan_enable=NULL;
	
	data->obaudrate = 600000;
	data->ibaudrate = 2000000;
	data->opackets = 0;
	data->ipackets = 0;
	data->obytes = 0;
	data->ibytes = 0;

	if(link_speed){
		if(get_wan_up(ext_if_name)){
			ethwan_enable = nvram_get("ethwan_enable");
			if(!ethwan_enable || strcmp(ethwan_enable, "0") == 0){
				get_adsl_rate(&data->ibaudrate, &data->obaudrate);
			}
			else{
				int wan_speed=get_ethernet_link();
				
				data->obaudrate=data->ibaudrate=wan_speed*1000*1000;
			}
			if(ethwan_enable){
				free(ethwan_enable);
				ethwan_enable=NULL;	
			}
		}
		else
			data->obaudrate=data->ibaudrate==0;
		r=0;
	}
	else{	
		f = fopen("/proc/net/dev", "r");
		if(!f) {
			syslog(LOG_ERR, "cannot open /proc/net/dev : %m");
			return -1;
		}
		/* discard the two header lines */
		fgets(line, sizeof(line), f);
		fgets(line, sizeof(line), f);
		while(fgets(line, sizeof(line), f)) {
			p = line;
			while(*p==' ')
				p++;
			i = 0;
			while(ifname[i] == *p){
				p++; i++;
			}
			/* TODO : how to handle aliases ? */
			if(ifname[i] || *p != ':')
				continue;
			p++;
			while(*p==' ')
				p++;
			data->ibytes = strtoul(p, &p, 0);
			while(*p==' ')
				p++;
			data->ipackets = strtoul(p, &p, 0);
			/* skip 6 columns */
			for(i=6; i>0 && *p!='\0'; i--) {
				while(*p==' ')
					p++;
				while(*p!=' ' && *p)
					p++;
			}
			while(*p==' ')
				p++;
			data->obytes = strtoul(p, &p, 0);
			while(*p==' ')
				p++;
			data->opackets = strtoul(p, &p, 0);
			r = 0;
			break;
		}
		fclose(f);
	}
	return r;
}

