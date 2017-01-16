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
 * From: @(#)output.c	5.15 (Berkeley) 2/28/91
 * From: @(#)output.c	8.1 (Berkeley) 6/5/93
 */
char output_rcsid[] = 
  "$Id: output.c,v 1.1.1.1 2009-01-05 09:01:16 fred_fu Exp $";

extern int broadcast;
/*
 * Routing Table Management Daemon
 */

#include "defs.h"

/*
 * Apply the function "f" to all non-passive
 * interfaces.  If the interface supports the
 * use of broadcasting use it, otherwise address
 * the output to the known router.
 */

void toall(void (*f)(struct sockaddr *, int, struct interface *, int), 
	int rtstate, struct interface *skipif)
{
    struct interface *ifp;
    struct sockaddr *dst;
    //int size = 0;
    int flags;
    struct sockaddr_in *ptemp,dst_ip;
    
    for (ifp = ifnet; ifp; ifp = ifp->int_next) {
        if (ifp->int_flags & IFF_PASSIVE || ifp == skipif)
            continue;
        dst = ifp->int_flags & IFF_BROADCAST ? &ifp->int_broadaddr :
            ifp->int_flags & IFF_POINTOPOINT ? &ifp->int_dstaddr :
            &ifp->int_addr;
        flags = ifp->int_flags & IFF_INTERFACE ? MSG_DONTROUTE : 0;
        
        /*!!! If rip v2(Only) dst is multicast address */
        if( comp_switch == RIP_V2 || comp_switch == RIP_V1_MIXED)
        {
            if(broadcast){
                int on = 1;
                setsockopt (sock, SOL_SOCKET, SO_BROADCAST, (char *) &on, sizeof on);
            }else{
                setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &ifp->int_addr, sizeof(ifp->int_addr));
            }
            dst_ip.sin_family = AF_INET;     /* host byte order */
            dst_ip.sin_port = rip_port; /* short, network byte order */
            if(broadcast){
                ptemp=(struct sockaddr_in *)&ifp->int_broadaddr;
                dst_ip.sin_addr.s_addr=ptemp->sin_addr.s_addr;
            }else
                dst_ip.sin_addr.s_addr = inet_addr(RIP_GROUP);
            
            dst = (struct sockaddr *)&dst_ip;
            flags = 0;
        }
        ptemp = (struct sockaddr_in *)dst;
        (*f)(dst, flags, ifp, rtstate);
    }
}

/*
 * Output a preformed packet.
 */

void sndmsg(struct sockaddr *dst, int flags, struct interface *ifp, int rtstate)
{
    int size = 0;
    (void)rtstate;
    size = (pwd==1)? sizeof( struct rip ) + sizeof(struct netinfo):
        sizeof( struct rip );
    (*afswitch[dst->sa_family].af_output)(sock, flags,
                                          dst, size);
    TRACE_OUTPUT(ifp, dst, size);
}

/*
 * to get all static route
 */
extern int myPipe(char *command, char **output);
char   static_route[25][20]={{0,},};
static int get_static_route(void)
{
    char *pt = NULL,*tmp_pt = NULL;
    char one_route[128],token=0x01;
    int  tmp_int,i,idx=0;
    char tmp_str[20];
    myPipe("/usr/sbin/nvram get rt_static_route",&pt);
    
    tmp_pt=pt;
    
    while(*pt!='\0'){
		if(idx>=25)
			break;
			
        i=0;
        while(*pt!=token && *pt!='\0')	one_route[i++]=*pt++;
        one_route[i]='\0';
        if(*pt!=0) pt++;

        sscanf(one_route,"%[^:]:%[^:]:%[^:]:%[^:]:%d:%d:%d:%[^:]:%d",
               tmp_str,
               static_route[idx],
               tmp_str,
               tmp_str,
               &tmp_int,
               &tmp_int,
               &tmp_int,
               tmp_str,
               &tmp_int);
        idx++;
    }
    if(tmp_pt!=NULL)
    {
        free(tmp_pt);
        tmp_pt=NULL;
    }
    return (idx-1);
}

#include <stdarg.h>
void mBUG(char *format,...){
    va_list args;
    FILE *fp;

    fp = fopen("/tmp/riplog","a+");
    if(!fp){
        return;
    }
    va_start(args,format);
    vfprintf(fp,format,args);
    va_end(args);
    fprintf(fp,"\n");
    fflush(fp);
    fclose(fp);
}

/*
 * Supply dst with the contents of the routing tables.
 * If this won't fit in one packet, chop it up into several.
 */
void supply(struct sockaddr *dst, int flags, struct interface *ifp, int rtstate)
{
    struct rt_entry *rt;
    struct netinfo *n = msg->rip_nets;
    struct sockaddr *interface;
    
    struct rthash *rh;
    struct rthash *base = hosthash;
    int doinghost = 1, size;
    void (*output)(int,int,struct sockaddr *,int) = 
        afswitch[dst->sa_family].af_output;
    int (*sendroute)(struct rt_entry *, struct sockaddr *) = 
        afswitch[dst->sa_family].af_sendroute;
    int npackets = 0;
    
    int idx=0,i=0,is_static_route=0;
    struct sockaddr_in *temp ;
    idx=get_static_route();
    
    msg->rip_cmd = RIPCMD_RESPONSE;
    msg->rip_vers = ( comp_switch == RIP_V1 || comp_switch == RIP_V2_MIXED) ? RIP_VERSION_1 : RIP_VERSION_2 ;
    
    memset(msg->rip_res1, 0, sizeof(msg->rip_res1));
    if( (( comp_switch > RIP_V1 )&&( comp_switch != RIP_V2_MIXED )) && ( pwd == PWD_YES ) )
    {
        /*Fill in authentication information */
        msg->rip_nets[0].sa_family = htons(AUTH_FAMILY);
        msg->rip_nets[0].route_tag = htons(AUTH_CLRTXT); /* Actually Authentication Type */
        strncpy((char *)(&msg->rip_nets[0].ip_addr),passwd,strlen(passwd));
        n = &msg->rip_nets[1];
    }
 again:
    for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++)
		for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw)
		{
            /*
             * Don't resend the information on the network
             * from which it was received (unless sending
             * in response to a query).
             */
            if (ifp && rt->rt_ifp == ifp &&
		    (rt->rt_state & RTS_INTERFACE) == 0)
                continue;
            if (rt->rt_state & RTS_EXTERNAL)
                continue;
			
	        /*
	         * if send to lan,should not contain static route(bug 0991)
	         */
	        if(ifp && ifp->int_name && (strncmp(ifp->int_name, "br0", 3)==0) )
	        {
	        	for(i=0;i<=idx;i++)
	        	{
	        	    if(strcmp(inet_ntoa(((struct sockaddr_in *)(&rt->rt_dst))->sin_addr),static_route[i])==0)
	        	    {
	        	    	is_static_route=1;
	        	    	break;
	        	    }
	        	}
	        	if(is_static_route==1)
	        	{
	        	    is_static_route=0;    //reset is_static_route flag
	        	    continue;
	        	}
	        }
			/*filter the net in which the ifp is*/
			if(ifp)
			{
		 		if(ifp->int_subnet == htonl(((struct sockaddr_in *)(&rt->rt_dst))->sin_addr.s_addr))
		  			continue;
			}
            /*
             * For dynamic updates, limit update to routes
             * with the specified state.
             */
            if (rtstate && (rt->rt_state & rtstate) == 0)
                continue;
            /*
             * Limit the spread of subnet information
             * to those who are interested.
             */
            /*!!! This check need not be done for RIP v2 mode */
            if (doinghost == 0 && rt->rt_state & RTS_SUBNET && (comp_switch != RIP_V2 &&comp_switch != RIP_V1_MIXED))
            {
                if (rt->rt_dst.sa_family != dst->sa_family)
                    continue;
                if( ifp == 0)
                    interface = dst;
                else
                {
                    /* Check if the SUBNET route should be sent on this interface */
		            interface = ifp->int_flags & IFF_BROADCAST ? &ifp->int_broadaddr :
                                ifp->int_flags & IFF_POINTOPOINT ? &ifp->int_dstaddr :
                                &ifp->int_addr;
                }
                if ((*sendroute)(rt, interface) == 0)
                    continue;
                /*if ((*sendroute)(rt, dst) == 0)
                  continue;*/
            }
            
            size = (char *)n - packet;
            if (size > MAXPACKETSIZE - (int)sizeof (struct netinfo))
            {
                
                TRACE_OUTPUT(ifp, dst, size);
                (*output)(sock, flags, dst, size);
                /*
                 * If only sending to ourselves,
                 * one packet is enough to monitor interface.
                 */
                if (ifp && (ifp->int_flags &
                            (IFF_BROADCAST | IFF_POINTOPOINT | IFF_REMOTE)) == 0)
                    return;
                n = msg->rip_nets;
                npackets++;
            }
            temp = (struct sockaddr_in *)(&rt->rt_dst);
            n->sa_family = rt->rt_dst.sa_family;
#if BSD < 198810
            if (sizeof(n->sa_family) > 1)	/* XXX */
                n->sa_family = htons(n->sa_family);
#endif
            n->rip_metric = htonl(rt->rt_metric);
            n->ip_addr =  temp->sin_addr.s_addr;
            if(  msg->rip_vers == RIP_VERSION_2 )
            {
                n->route_tag = htons(rt->rt_tag);
                if( rt->rt_netmask.s_addr == 0xFFFFFFFF) n->subnetmask = 0;
                else n->subnetmask = rt->rt_netmask.s_addr;
                /* If router's subnet is not same as this IF's subnet send
                   if IP address as router */
                temp = (struct sockaddr_in *)(&rt->rt_router);
                n->nexthop_ip = temp->sin_addr.s_addr;
/*
                if(ifp&& (htonl(n->nexthop_ip) & ifp->int_subnetmask) != ifp->int_subnet)
                {
                    temp = (struct sockaddr_in *)(&ifp->int_addr);
                    n->nexthop_ip = temp->sin_addr.s_addr;
                }else{
                    n->nexthop_ip = 0;
                }
*/            
               if(!(ifp&& ((htonl(n->nexthop_ip) & ifp->int_subnetmask) == ifp->int_subnet)
				    && htonl(n->nexthop_ip)!=((struct sockaddr_in *)(&ifp->int_addr))->sin_addr.s_addr))
			        n->nexthop_ip = 0;
            }
            else
            {
                /* Respond with the standard RIP v1 response packet */
                n->route_tag = 0;
                n->subnetmask = 0;
                n->nexthop_ip = 0;
            }
            n++;
		}
    
    if (doinghost)
    {
        doinghost = 0;
        base = nethash;
        goto again;
    }
    
    /* add default routing info */
    //not send default route on the side of wan (bug 0973)
    if(ifp && ifp->int_name && (strncmp(ifp->int_name, "br0", 3)==0) )
    {
        size = (char *)n - packet;
        if (size > MAXPACKETSIZE - (int)sizeof (struct netinfo))
        {
            TRACE_OUTPUT(ifp, dst, size);
            (*output)(sock, flags, dst, size);
            /*
             * If only sending to ourselves,
             * one packet is enough to monitor interface.
             */
            if (ifp && (ifp->int_flags &
                        (IFF_BROADCAST | IFF_POINTOPOINT | IFF_REMOTE)) == 0)
                return;
            n = msg->rip_nets;
            npackets++;
        }
        n->route_tag = 0;
        n->subnetmask = 0;
        n->nexthop_ip = 0;
        n->ip_addr = 0;
        n->sa_family = htons(2);
        n->rip_metric = htonl(1);
        n++;
    }

    if (n != msg->rip_nets || (npackets == 0 && rtstate == 0))
    {
        size = (char *)n - packet;
        TRACE_OUTPUT(ifp, dst, size);
        (*output)(sock, flags, dst, size);
    }
}
