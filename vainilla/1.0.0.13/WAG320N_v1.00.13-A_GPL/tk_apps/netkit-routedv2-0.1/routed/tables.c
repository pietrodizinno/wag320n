/*
 * Copyright (c) 1983, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * From: @(#)tables.c	5.17 (Berkeley) 6/1/90
 * From: @(#)tables.c	8.1 (Berkeley) 6/5/93
 */
char tables_rcsid[] = 
  "$Id: tables.c,v 1.1.1.1 2009-01-05 09:01:16 fred_fu Exp $";


/*
 * Routing Table Management Daemon
 */

#include "defs.h"
#include <sys/ioctl.h>
#include <syslog.h>
#include <errno.h>
#include <search.h>
#define dsyslog(f,args...)
#ifndef DEBUG
#define	DEBUG	0
#endif

#define FIXLEN(s) { }

static int install = !DEBUG;		/* if 1 call kernel */
struct rthash nethash[ROUTEHASHSIZ];
struct rthash hosthash[ROUTEHASHSIZ];

extern unsigned long int gl_subnet[ROUTEHASHSIZ];
extern int gl_route_size;
extern char current_route[ROUTEHASHSIZ][20];
extern char next_hop[];
extern int myPipe(char *command, char **output);

static void printroute(struct rt_entry *rt)
{
    struct sockaddr_in *temp;

    temp = (struct sockaddr_in *)(&rt->rt_rtu.rtu_entry.rtu_dst);
    printf("printroute:Destination = %s\n",inet_ntoa(temp->sin_addr));
    temp = (struct sockaddr_in *)(&rt->rt_rtu.rtu_entry.rtu_router);
    printf("printroute:Router = %s\n",inet_ntoa(temp->sin_addr));
    printf("printroute:rtu_rtflags = %d\n",rt->rt_rtu.rtu_entry.rtu_rtflags);
    printf("printroute:rtu_metric = %d\n",rt->rt_rtu.rtu_entry.rtu_metric);
    printf("printroute:rtu_ifmetric = %d\n",rt->rt_rtu.rtu_entry.rtu_ifmetric);
    printf("printroute:Netmask = %s\n",inet_ntoa(rt->rt_rtu.rtu_entry.rtu_netmask));
    printf("printroute:rtu_tag = %d\n",rt->rt_rtu.rtu_entry.rtu_tag);
}

/* to judge a netmask is logical or illogical ,illogical return 0 (12-07-07) */
#if 0
int legal_netmask(char *netmask_str)
{
    int  dec,i;
    int index=7;
    char bin_ip_segment[9] = "00000000";
    char bin_netmask[40]={0,};


    char ip_segment[4][4]={{0,},};
    sscanf(netmask_str,"%[^.].%[^.].%[^.].%s",ip_segment[0],ip_segment[1],ip_segment[2],ip_segment[3]);
    
    for(i=0;i<4;i++)
    {
        /* reset index and bin_ip_segment */
        index=7;
        strcpy(bin_ip_segment,"00000000");
    	dec=atoi(ip_segment[i]);
   	
        if (dec == 1 )
        {
            strcpy(bin_ip_segment,"00000001");
            sprintf(bin_netmask,"%s%s",bin_netmask,bin_ip_segment);
            continue;
        }
        while(1)
        {
            if (dec >= 2)
            {
                bin_ip_segment[index]=dec%2+48;
                dec = dec/2;
                index--;
            }
            else
            {
                bin_ip_segment[index]=dec+48;
                sprintf(bin_netmask,"%s%s",bin_netmask,bin_ip_segment);
                break;
            }
        }
    }
//    printf("bin_netmask = %s\n",bin_netmask);
    
    for(i=0;bin_netmask[i+1]!='\0';i++)
    {
    	if(bin_netmask[i]=='0'&&bin_netmask[i+1]=='1')
    		return 0;
    }
}
#endif


/* to judge the nexthop is logical or illogical ,illogical return 0 (12-07-18) */
int legal_nexthop(void)
{
    char *lan_ip = NULL,*tmp_lanip = NULL;
    char *wan_ip = NULL,*tmp_wanip = NULL;
    char ip_addr[4][6]={{0,},};
    
    myPipe("/usr/sbin/nvram get lan_ipaddr",&lan_ip);
    tmp_lanip=lan_ip;
    if(lan_ip!=NULL)
        lan_ip=lan_ip+5;
        
    if(access("/tmp/wan_uptime",F_OK)!=0)
    	wan_ip=NULL;
    else    
    {
        myPipe("cat /tmp/wan_ipaddr",&wan_ip);
        tmp_wanip=wan_ip;
    }
    
    sscanf(next_hop,"%[^.].%[^.].%[^.].%s",ip_addr[0],ip_addr[1],ip_addr[2],ip_addr[3]);
    
    if( strncmp(next_hop,"127",3)==0         /* loop ip */
      ||( atoi(ip_addr[0])>=224 && atoi(ip_addr[0])<240 )      /* mutilcast ip */
      ||strncmp(next_hop,lan_ip,strlen(next_hop))==0           /* lan ip */
      ||strcmp(ip_addr[3],"255")==0           /* broadcast ip (need improve)*/
      )
    {
        if(tmp_lanip!=NULL)
        	free(tmp_lanip);
        if(tmp_wanip!=NULL)
        	free(tmp_wanip);
        return 0;
    }
    
    if(wan_ip!=NULL && strcmp(next_hop,wan_ip)==0)  /* wan ip */
    {
        if(tmp_lanip!=NULL)
        	free(tmp_lanip);
        if(tmp_wanip!=NULL)
        	free(tmp_wanip);
    	return 0;
    }
    
    if(tmp_lanip!=NULL)
        free(tmp_lanip);
    if(tmp_wanip!=NULL)
        free(tmp_wanip);    
    return 1;

}



/*
 * Lookup dst in the tables for an exact match.
 */
struct rt_entry *rtlookup(struct sockaddr *dst, struct in_addr *netmask, unsigned short tag)
{
    register struct rt_entry *rt;
    register struct rthash *rh;
    register u_int hash;
    struct afhash h;
    int doinghost = 1;

    /*
      printf("rtlookup:netmask = %s\n",inet_ntoa(*netmask));
      printf("rtlookup:tag = %d\n",tag);
      printf("rtlookup:destination = %s\n",inet_ntoa( ((struct sockaddr_in *)(dst))->sin_addr)); */

    if (dst->sa_family >= af_max)
        return (0);
    (*afswitch[dst->sa_family].af_hash)(dst, &h);
    hash = h.afh_hosthash;
    rh = &hosthash[hash & ROUTEHASHMASK];
 again:
    for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
        if (rt->rt_hash != hash)
            continue;
    	printroute(rt);
		//if (equal(&rt->rt_dst, dst) && ( htonl(netmask->s_addr) == rt->rt_netmask.s_addr) && (tag == rt->rt_tag))
		if (equal(&rt->rt_dst, dst) && (tag == rt->rt_tag))
		{
			printroute(rt);
            return (rt);
        }
    }
    if (doinghost) {
        doinghost = 0;
        hash = h.afh_nethash;
        rh = &nethash[hash & ROUTEHASHMASK];
        goto again;
    }
    return (0);
}

struct sockaddr wildcard;	/* zero valued cookie for wildcard searches */

/*
 * Find a route to dst as the kernel would.
 */

struct rt_entry *rtfind(struct sockaddr *dst,struct in_addr *netmask , unsigned short tag)
{
    register struct rt_entry *rt;
    register struct rthash *rh;
    register u_int hash;
    struct afhash h;
    int af = dst->sa_family;
    int doinghost = 1, (*match)(struct sockaddr *,struct sockaddr *)=NULL;

    /*printf("rtfind:netmask = %s\n",inet_ntoa(*netmask));
      printf("rtfind:tag = %d\n",tag);
      printf("rtfind:destination = %s\n",inet_ntoa( ((struct sockaddr_in *)(dst))->sin_addr)); */

    if (af >= af_max)
        return (0);
    (*afswitch[af].af_hash)(dst, &h);
    hash = h.afh_hosthash;
    rh = &hosthash[hash & ROUTEHASHMASK];

 again:
    for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
        if (rt->rt_hash != hash)
            continue;
        if (doinghost) {
            if (equal(&rt->rt_dst, dst) &&
                (netmask->s_addr == rt->rt_netmask.s_addr) &&
                (tag == rt->rt_tag))
            {
                printroute(rt);
                return (rt);
            }
        } else {
            if (rt->rt_dst.sa_family == af &&
                (*match)(&rt->rt_dst, dst))
                return (rt);
        }
    }
    if (doinghost) {
        doinghost = 0;
        hash = h.afh_nethash;
        rh = &nethash[hash & ROUTEHASHMASK];
        match = afswitch[af].af_netmatch;
        goto again;
    }
    return (0);
}


void rtadd(struct sockaddr *dst, struct sockaddr *gate, int metric, int state, struct in_addr *netmask , unsigned short tag)
{
    struct afhash h;
    register struct rt_entry *rt;
    struct rthash *rh;
    int af = dst->sa_family, flags;
    u_int hash;
    //char buf1[256], buf2[256];
    unsigned long subnet = 0,mask = 0;
  int i_cursor;
    /*
      printf("rtadd:netmask = %s\n",inet_ntoa(*netmask));
      printf("rtadd:tag = %d\n",tag);
      printf("rtadd:metric = %d\n",metric);
      printf("rtadd:destination = %s\n",inet_ntoa( ((struct sockaddr_in *)(dst))->sin_addr));
      printf("rtadd:router = %s\n",inet_ntoa( ((struct sockaddr_in *)(gate))->sin_addr));
    */
    /*we must not allow dut add default route from rip message*/
    /* and not allow dut add 0.0.0.0 */
    if( strcmp(inet_ntoa(((struct sockaddr_in *)(dst))->sin_addr),"0.0.0.0")==0 )
        return;
    if(strcmp(inet_ntoa(((struct sockaddr_in *)(dst))->sin_addr),"127.0.0.1")==0)
        return;			

    /* dst&netmask!=dst (invalid) return */
    if(netmask->s_addr!=0 &&((((struct sockaddr_in *)(dst))->sin_addr.s_addr&htonl(netmask->s_addr)) != ((struct sockaddr_in *)(dst))->sin_addr.s_addr) )
    {
        return;
    }

    /* ilegal netmask ,return */
//    if(legal_netmask(inet_ntoa(*netmask))!=1)
//    {
//    	LogMsg("ilegal netmask ,return\n");
//    	return;
//    }
    /* unreasonable next hop ,return */
    if(legal_nexthop()!=1)
    {
    	return;
    }
    

    if (af >= af_max)
        return;
    (*afswitch[af].af_hash)(dst, &h);
    flags = (*afswitch[af].af_rtflags)(dst);

    /* If netmask != 0 determine if the route is for host or network else
       leave it alone */
    if( netmask->s_addr != 0)
    {
        subnet = ntohl(((struct sockaddr_in *)dst)->sin_addr.s_addr);
        mask = netmask->s_addr;
        if( (subnet - (subnet&mask)) == 0)
            flags &= (~RTF_HOST) & (RTF_SUBNET);
        else
            flags &= (RTF_HOST) & (~RTF_SUBNET);
    }
    /*
     * Subnet flag isn't visible to kernel, move to state.	XXX
     */
    FIXLEN(dst);
    FIXLEN(gate);
    if (flags & RTF_SUBNET) {
        state |= RTS_SUBNET;
        flags &= ~RTF_SUBNET;
    }
    if (flags & RTF_HOST) {
        hash = h.afh_hosthash;
        rh = &hosthash[hash & ROUTEHASHMASK];
    } else {
        hash = h.afh_nethash;
        rh = &nethash[hash & ROUTEHASHMASK];
    }
    rt = (struct rt_entry *)malloc(sizeof (*rt));
    if (rt == 0)
        return;
    rt->rt_hash = hash;
    rt->rt_dst = *dst;
    
    if( strcmp(next_hop,"0.0.0.0")!=0 )
        ((struct sockaddr_in *)gate)->sin_addr.s_addr=inet_addr(next_hop);
        
    rt->rt_router = *gate;
    rt->rt_timer = 0;
    rt->rt_flags = RTF_UP | flags;
    rt->rt_state = state | RTS_CHANGED;
    rt->rt_ifp = if_ifwithdstaddr(&rt->rt_dst);
    if (rt->rt_ifp == 0)
        rt->rt_ifp = if_ifwithnet(&rt->rt_router);
    if ((state & RTS_INTERFACE) == 0)
        rt->rt_flags |= RTF_GATEWAY;
    rt->rt_metric = metric;
    rt->rt_netmask.s_addr = htonl(netmask->s_addr); /*!!! */
    rt->rt_tag = htons(tag); /*!!! */
    printroute(rt);
    insque((struct qelem *)rt, (struct qelem *)rh);
	TRACE_ACTION("ADD", rt);
	/*if route table have it, don't add this route*/
	for(i_cursor=0;i_cursor<gl_route_size;i_cursor++){
#if 0
	    if(gl_subnet[i_cursor] == (((struct sockaddr_in *)dst)->sin_addr.s_addr)){
#endif
        if( strcmp(current_route[i_cursor],inet_ntoa((((struct sockaddr_in *)dst)->sin_addr)))==0){
	    	return;
	    }
	  }
	  
	/*
     * If the ioctl fails because the gateway is unreachable
     * from this host, discard the entry.  This should only
     * occur because of an incorrect entry in /etc/gateways.
     */
    if ((rt->rt_state & (RTS_INTERNAL | RTS_EXTERNAL)) == 0 &&
        rtioctl(ADD, &rt->rt_rt ) < 0) {
        if (errno != EEXIST && gate->sa_family < af_max){
            dsyslog(LOG_ERR,
                    "adding route to net/host %s through gateway %s: %m\n",
                    (*afswitch[dst->sa_family].af_format)(dst, buf1,
                                                          sizeof(buf1)),
                    (*afswitch[gate->sa_family].af_format)(gate, buf2,
                                                           sizeof(buf2)));
        }
        perror("ADD ROUTE");
        if (errno == ENETUNREACH) {
            TRACE_ACTION("DELETE", rt);
            remque((struct qelem *)rt);
            free((char *)rt);
        }
    }
}

void rtchange(struct rt_entry *rt, struct sockaddr *gate, short metric,struct in_addr *netmask , unsigned short tag)
{
    int add = 0, delete = 0, newgateway = 0;
    struct rtuentry oldroute;

    FIXLEN(gate);
    FIXLEN(&(rt->rt_router));
    FIXLEN(&(rt->rt_dst));

    if (!equal(&rt->rt_router, gate) || ( netmask->s_addr != rt->rt_netmask.s_addr) || ( tag != rt->rt_tag)) {
        newgateway++;
        TRACE_ACTION("CHANGE FROM ", rt);
    } else if (metric != rt->rt_metric)
    {
        TRACE_NEWMETRIC(rt, metric);
        /* added 12-10-07 if metric been changed,need change this route */
        if(metric!=HOPCNT_INFINITY)
        	newgateway++;       	
    }
        
    if ((rt->rt_state & RTS_INTERNAL) == 0) {
        /*
         * If changing to different router, we need to add
         * new route and delete old one if in the kernel.
         * If the router is the same, we need to delete
         * the route if has become unreachable, or re-add
         * it if it had been unreachable.
         */
        if (newgateway) {
            add++;
            if (rt->rt_metric != HOPCNT_INFINITY)
                delete++;
        } else if (metric == HOPCNT_INFINITY)
            delete++;
        else if (rt->rt_metric == HOPCNT_INFINITY)
            add++;
    }
    /* Linux 1.3.12 and up */
    if (kernel_version >= 0x01030b) {
        if (add && delete && rt->rt_metric == metric)
            delete = 0;
    } else {
	/* Linux 1.2.x and 1.3.7 - 1.3.11 */
        if (add && delete)
            delete = 0;
    }

    if (delete)
        oldroute = rt->rt_rt;
    if ((rt->rt_state & RTS_INTERFACE) && delete) {
        rt->rt_state &= ~RTS_INTERFACE;
        rt->rt_flags |= RTF_GATEWAY;
        if (metric > rt->rt_metric && delete){
            dsyslog(LOG_ERR, "%s route to interface %s (timed out)",
                    add? "changing" : "deleting",
                    rt->rt_ifp ? rt->rt_ifp->int_name : "?");
        }
    }
    if (add) {
        rt->rt_router = *gate;
        rt->rt_ifp = if_ifwithdstaddr(&rt->rt_router);
        if (rt->rt_ifp == 0)
            rt->rt_ifp = if_ifwithnet(&rt->rt_router);
    }
    rt->rt_metric = metric;
    rt->rt_state |= RTS_CHANGED;
    if (newgateway)
        TRACE_ACTION("CHANGE TO   ", rt);
    if (add && rtioctl(ADD, &rt->rt_rt) < 0)
        perror("ADD ROUTE");
    if (delete && rtioctl(DELETE, &oldroute) < 0)
        perror("DELETE ROUTE");
}

void rtdelete(struct rt_entry *rt)
{

    TRACE_ACTION("DELETE", rt);
    FIXLEN(&(rt->rt_router));
    FIXLEN(&(rt->rt_dst));
    if (rt->rt_metric < HOPCNT_INFINITY) {
        if ((rt->rt_state & (RTS_INTERFACE|RTS_INTERNAL)) == RTS_INTERFACE){
            dsyslog(LOG_ERR,
		    "deleting route to interface %s? (timed out?)",
		    rt->rt_ifp->int_name);
        }
        if ((rt->rt_state & (RTS_INTERNAL | RTS_EXTERNAL)) == 0 &&
            rtioctl(DELETE, &rt->rt_rt) < 0)
            perror("rtdelete");
    }
    remque((struct qelem *)rt);
    free((char *)rt);
}

void rtdeleteall(int sig)
{
    register struct rthash *rh;
    register struct rt_entry *rt;
    struct rthash *base = hosthash;
    int doinghost = 1, i;

 again:
    for (i = 0; i < ROUTEHASHSIZ; i++) {
        rh = &base[i];
        rt = rh->rt_forw;
        for (; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
            if (rt->rt_state & RTS_INTERFACE ||
                rt->rt_metric >= HOPCNT_INFINITY)
                continue;
            TRACE_ACTION("DELETE", rt);
            if ((rt->rt_state & (RTS_INTERNAL|RTS_EXTERNAL)) == 0 &&
                rtioctl(DELETE, &rt->rt_rt) < 0)
                perror("rtdeleteall");
        }
    }
    if (doinghost) {
        doinghost = 0;
        base = nethash;
        goto again;
    }
//    exit(sig);
}

/*
 * If we have an interface to the wide, wide world,
 * add an entry for an Internet default route (wildcard) to the internal
 * tables and advertise it.  This route is not added to the kernel routes,
 * but this entry prevents us from listening to other people's defaults
 * and installing them in the kernel here.
 */

void rtdefault(void)
{
    struct in_addr mask;
    mask.s_addr = 0xFFFFFFFF;
    printf ("rtdefault enter\n");
    rtadd((struct sockaddr *)&inet_default, 
          (struct sockaddr *)&inet_default, 1,
          RTS_CHANGED | RTS_PASSIVE | RTS_INTERNAL,&mask,0);
    printf ("rtdefault leave\n");
}

void rtinit(void)
{
    register struct rthash *rh;
    int i;

    for (i = 0; i < ROUTEHASHSIZ; i++) {
        rh = &nethash[i];
        rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
    }
    for (i = 0; i < ROUTEHASHSIZ; i++) {
        rh = &hosthash[i];
        rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
    }
}

int rtioctl(int action, struct rtuentry *ort)
{
    struct rtentry rt;
    unsigned int netmask;
    unsigned int dst;

    if (install == 0)
        return (errno = 0);

#undef rt_flags
#undef rt_ifp
#undef rt_metric
#undef rt_dst

    rt.rt_flags = (ort->rtu_flags & (RTF_UP|RTF_GATEWAY|RTF_HOST));
    rt.rt_metric = ort->rtu_metric;
    rt.rt_dev = NULL;
    rt.rt_dst = *(struct sockaddr *)&ort->rtu_dst;
    dst = ((struct sockaddr_in *)&rt.rt_dst)->sin_addr.s_addr;
    rt.rt_gateway = *(struct sockaddr *)&ort->rtu_router;
    if (rt.rt_flags & RTF_HOST)
        netmask = 0xffffffff;
    else
    {
        if( (int)(ort->rtu_netmask.s_addr) == 0 )
            netmask = inet_maskof(dst);
        else
            netmask = (unsigned long)ort->rtu_netmask.s_addr;
    }
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_family = AF_INET;
    ((struct sockaddr_in *)&rt.rt_genmask)->sin_addr.s_addr = netmask;
	
    if (traceactions) {
        fprintf(ftrace, "rtioctl %s %08lx/%08lx\n",
                action == ADD ? "ADD" : "DEL",
                (unsigned long int)ntohl(dst),
                (unsigned long int)ntohl(netmask));
        fflush(ftrace);
    }

    switch (action) {

    case ADD:
        return (ioctl(sock, SIOCADDRT, (char *)&rt));

    case DELETE:
        return (ioctl(sock, SIOCDELRT, (char *)&rt));

    default:
        return (-1);
    }
}
