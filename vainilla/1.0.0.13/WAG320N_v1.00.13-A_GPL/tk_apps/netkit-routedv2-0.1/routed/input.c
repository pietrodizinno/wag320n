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

#undef DEBUG

#ifdef DEBUG
#define DEBUGP printf
#else
#define DEBUGP(format, args...)
#endif

/*
 * From: @(#)input.c	5.22 (Berkeley) 6/1/90
 * From: @(#)input.c	8.1 (Berkeley) 6/5/93
 */
char input_rcsid[] = 
  "$Id: input.c,v 1.1.1.1 2009-01-05 09:01:16 fred_fu Exp $";


/*
 * Routing Table Management Daemon
 */

#include "defs.h"
#include <syslog.h>
#define dsyslog(f,args...)
struct timeval now;		/* current idea of time */
struct timeval lastbcast;	/* last time all/changes broadcast */
struct timeval lastfullupdate;	/* last time full table broadcast */
struct timeval nextbcast;    /* time to wait before changes broadcast */
int needupdate;		    /* true if we need update at nextbcast */

char next_hop[20]="0.0.0.0";

static void printnetinfo( struct netinfo *n)
{
    struct sockaddr_in temp;
    printf("Family = %d\n",htons(n->sa_family));
    printf("Route Tag = %d\n",htons(n->route_tag));
    temp.sin_addr.s_addr = n->ip_addr;
    printf("IP Address = %s\n",inet_ntoa(temp.sin_addr));
    temp.sin_addr.s_addr = n->subnetmask;
    printf("Subnet Mask = %s\n",inet_ntoa(temp.sin_addr));
    temp.sin_addr.s_addr = n->nexthop_ip;
    printf("Next Hop Router = %s\n",inet_ntoa(temp.sin_addr));
    printf("Metric = %d\n",htonl(n->rip_metric));
    return;
}

static void printauthinfo( struct netinfo *n)
{
    printf("printauthinfo:Family = %d\n",htons(n->sa_family));
    printf("printauthinfo:Route Tag = %d\n",htons(n->route_tag));
    printf("printauthinfo:Password = %s\n",(char *)(&n->ip_addr));
    printf("printauthinfo:Our Password = %s\n",passwd);
}

static int check_pwd(struct rip *rip)
{
    struct netinfo *n;

    n = rip->rip_nets;
    
    //printauthinfo(n);
    /* Address familiy should be 0xFFFF */
    if( ( htons(n->sa_family) != AUTH_FAMILY) ||
        ( htons(n->route_tag) != AUTH_CLRTXT) ||
        (strncmp( (char *)(&n->ip_addr) , passwd , strlen(passwd)) != 0)  )
        return -1;
    return 0; /* OK */
}
/*
 * Process a newly received packet.
 */

void rip_input(struct sockaddr *from, struct rip *rip, int size)
{
    struct rt_entry *rt;
    struct netinfo *n;

    struct interface *ifp;
    int count, changes = 0;
    struct afswitch *afp;
    static struct sockaddr badfrom, badfrom2;
    char buf1[256], buf2[256];
    struct sockaddr rip_dest;
    struct sockaddr_in *ptemp,temp;

    /* for SCM CDRouter test */
    /* DUT doesn't accept the rip packet which is larger than 512(MAXPACKETSIZE) */    
    if(size>MAXPACKETSIZE)
        return;

    /*  SCM CDRouter test: ignores RIPv1 RIP route whose must be zero zone is nonzero */
    /* check zero zone*/
    char rip_info[MAXPACKETSIZE+1]={0,};
    char zero[8]={0,};
#define    TWO    2
#define    EIGHT  8
    memcpy(rip_info,rip,size);
    
    if(rip_info[1]==0x01)    //ripv1
    {    		
        if( memcmp(rip_info+2,zero,TWO)!=0
         ||memcmp(rip_info+6,zero,TWO)!=0
         ||memcmp(rip_info+12,zero,EIGHT)!=0)
        {
             DEBUGP("ripv1 and zero zone is not zero(error ripv1 packet)!\n");
             return;
        }
    }
    DEBUGP("RIP INPUT !\n");
    ifp = 0;
    
    temp.sin_addr.s_addr = ((struct sockaddr_in *)from)->sin_addr.s_addr;

    TRACE_INPUT(ifp, from, (char *)rip, size);
    if (from->sa_family >= af_max ||
        (afp = &afswitch[from->sa_family])->af_hash == (void (*)(struct sockaddr *, struct afhash *))0) {
        dsyslog(LOG_INFO,
                "\"from\" address in unsupported address family (%d), cmd %d\n",
                from->sa_family, rip->rip_cmd);
        return;
    }
	if (rip->rip_vers == 0 || rip->rip_vers == 3) {
        dsyslog(LOG_ERR,
                "RIP version 0 packet received from %s! (cmd %d)",
                (*afswitch[from->sa_family].af_format)(from, buf1,
                                                       sizeof(buf1)),
                rip->rip_cmd);
        return;
    }
    
    DEBUGP("Check for compatibility !\n");
    /* Check for compatibility */
    if( ((comp_switch == RIP_V1 || comp_switch == RIP_V1_MIXED) && (rip->rip_vers != RIP_VERSION_1))
      ||((comp_switch == RIP_V2 || comp_switch == RIP_V2_MIXED) && (rip->rip_vers != RIP_VERSION_2)/*&& (rip->rip_cmd!=RIPCMD_RESPONSE)*/ ) /*ripv2 can receive v1 packet*/ /* now not allowed*/
      )
        return;
    
    DEBUGP("RIP version %d packet received!\n",rip->rip_vers);

    /* If compatibility mode is RIPv2 or RIPv1 Compatible
       and authentication is enabled check for password in the packet */
    if( ((comp_switch > RIP_V1)&&(comp_switch != RIP_V1_MIXED)) && ( pwd == PWD_YES) &&
        ( rip->rip_vers == RIP_VERSION_2))
    {
        if(check_pwd(rip) < 0) /* Password check failed */
            return;
        n = &rip->rip_nets[1];
    }
    else
        n = rip->rip_nets;

    switch (rip->rip_cmd) {
    case RIPCMD_REQUEST:
        count = size - ((char *)n - (char *)rip);
        if (count < (int)sizeof (struct netinfo))
        {
            return;
        }
        for (; count > 0; n++) {
            if (count < (int)sizeof (struct netinfo))
                break;
            count -= sizeof (struct netinfo);
            //printnetinfo(n);
#if BSD < 198810
            if (sizeof(n->sa_family) > 1)	/* XXX */
                n->sa_family = ntohs(n->sa_family);
#endif
            n->rip_metric = ntohl(n->rip_metric);
            /* 
             * A single entry with sa_family == AF_UNSPEC and
             * metric ``infinity'' means ``all routes''.
             * We respond to routers only if we are acting
             * as a supplier, or to anyone other than a router
             * (eg, query).
             */
            if (n->sa_family == AF_UNSPEC &&
                n->rip_metric == HOPCNT_INFINITY && count == 0)
            {
                //CD router require we respond....
		//	  if (supplier || (*afp->af_portmatch)(from) == 0)
                //{
                supply(from, 0, 0, 0);
                //}
                return;
            }
            if (n->sa_family < af_max &&
                afswitch[n->sa_family].af_hash)
            {
                ptemp = (struct sockaddr_in *)&rip_dest;
                memset(ptemp,0x00,sizeof(struct sockaddr));
                ptemp->sin_family = n->sa_family;
                ptemp->sin_addr.s_addr = n->ip_addr;
                rt = rtlookup(&rip_dest, (struct in_addr *)&n->subnetmask, n->route_tag);
            }
            else
				return;
#define min(a, b) (a < b ? a : b)
            n->rip_metric = rt == 0 ? HOPCNT_INFINITY :
                min(rt->rt_metric + 1, HOPCNT_INFINITY);
#if BSD < 198810
            if (sizeof(n->sa_family) > 1)	/* XXX */
                n->sa_family = htons(n->sa_family);
#endif
            n->rip_metric = htonl(n->rip_metric);
            if( (comp_switch == RIP_V1) || (comp_switch == RIP_V2_MIXED)||((comp_switch == RIP_V1_COMP) &&
                                            ( rip->rip_vers == RIP_VERSION_1)) )
            {
                /* Respond with the standard RIP v1 response packet */
                n->route_tag = 0;
                n->subnetmask = 0;
                n->nexthop_ip = 0;
                DEBUGP("Respond with the standard RIP v1 response packet\n");
            }
            else if(rt!=0)
            {
                n->route_tag = htons(rt->rt_rtu.rtu_entry.rtu_tag);
        //n->subnetmask = htonl(rt->rt_rtu.rtu_entry.rtu_netmask.s_addr);
        n->subnetmask = rt->rt_rtu.rtu_entry.rtu_netmask.s_addr;/*for route specific request*/
        temp.sin_addr.s_addr = ((struct sockaddr_in *)(&rt->rt_rtu.rtu_entry.rtu_router))->sin_addr.s_addr;
        //n->nexthop_ip = temp.sin_addr.s_addr;
        n->nexthop_ip = 0;/*for route specific request*/
                DEBUGP("Respond with the standard RIP v2 response packet\n");
            }
        }
        rip->rip_cmd = RIPCMD_RESPONSE;
        /* Warning! it is HERE that the rip structure is filled out with filename information */
        memcpy(packet, rip, size);
        (*afp->af_output)(sock, 0, from, size);
        return;
        
    case RIPCMD_TRACEON:
    case RIPCMD_TRACEOFF:
        /* verify message came from a privileged port */
        if ((*afp->af_portcheck)(from) == 0)
            return;
        if ((ifp = if_iflookup(from)) == 0) {
            dsyslog(LOG_ERR, "trace command from unknown router, %s",
                    (*afswitch[from->sa_family].af_format)(from, buf1,
                                                           sizeof(buf1)));
            return;
        }
        
        if ((ifp->int_flags & 
             (IFF_BROADCAST|IFF_POINTOPOINT|IFF_REMOTE)) == 0) {
            dsyslog(LOG_ERR,
                    "trace command from router %s, with bad flags %x",
                    (*afswitch[from->sa_family].af_format)(from, buf1,
                                                           sizeof(buf1)),
                    ifp->int_flags);
            return;
        }
        
        if ((ifp->int_flags & IFF_PASSIVE) != 0) {
            dsyslog(LOG_ERR,
                    "trace command from  %s on a passive interface",
                    (*afswitch[from->sa_family].af_format)(from, buf1,
                                                           sizeof(buf1)));
            return;
        }

        ((char *)rip)[size] = '\0';
        if (rip->rip_cmd == RIPCMD_TRACEON)
            traceon(rip->rip_tracefile);
        else
            traceoff();
        return;
        
    case RIPCMD_RESPONSE:
        if( receiver == RECEIVER_NO )
            return;
        /* verify message came from a router */
        if ((*afp->af_portmatch)(from) == 0)
            return;
        (*afp->af_canon)(from);
        /* are we talking to ourselves? */
        ifp = if_ifwithaddr(from);
        if (ifp) {
            if (ifp->int_flags & IFF_PASSIVE) {
                dsyslog(LOG_ERR,
                        "bogus input (from passive interface, %s)",
                        (*afswitch[from->sa_family].af_format)(from,
                                                               buf1, sizeof(buf1)));
                return;
            }
            rt = rtfind(from, (struct in_addr *)&ifp->int_subnetmask, 0);
            if (rt == 0 || (((rt->rt_state & RTS_INTERFACE)==0) &&
                            rt->rt_metric >= ifp->int_metric))
                addrouteforif(ifp,0);
            else
                rt->rt_timer = 0;
            return;
        }
        /*
         * Update timer for interface on which the packet arrived.
         * If from other end of a point-to-point link that isn't
         * in the routing tables, (re-)add the route.
         */
        if ((rt = rtfind(from, (struct in_addr *)&ifp->int_subnetmask, 0)) &&
            (rt->rt_state & (RTS_INTERFACE | RTS_REMOTE)))
            rt->rt_timer = 0;
        
        else if ((ifp = if_ifwithdstaddr(from)) &&
                 (rt == 0 || rt->rt_metric >= ifp->int_metric))
            addrouteforif(ifp,0);
        
        /*
         * "Authenticate" router from which message originated.
         * We accept routing packets from routers directly connected
         * via broadcast or point-to-point networks,
         * and from those listed in /etc/gateways.
         */
        if ((ifp = if_iflookup(from)) == 0 || (ifp->int_flags &
                                               (IFF_BROADCAST | IFF_POINTOPOINT | IFF_REMOTE)) == 0 ||
            ifp->int_flags & IFF_PASSIVE) {
            if (memcmp(from, &badfrom, sizeof(badfrom)) != 0) {
                dsyslog(LOG_ERR,
                        "packet from unknown router, %s",
                        (*afswitch[from->sa_family].af_format)(from,
                                                               buf1, sizeof(buf1)));
                badfrom = *from;
            }
            return;
        }
        size -= 4 * sizeof (char);
        n = rip->rip_nets;
        for (; size > 0; size -= sizeof (struct netinfo), n++) {
            if (size < (int)sizeof (struct netinfo))
                break;
            //printnetinfo(n);
#if BSD < 198810
            if (sizeof(n->sa_family) > 1)	/* XXX */
                n->sa_family =
                    ntohs(n->sa_family);
#endif
            n->rip_metric = ntohl(n->rip_metric);
            if (n->sa_family >= af_max ||
                (afp = &afswitch[n->sa_family])->af_hash ==
                (void (*)(struct sockaddr *,struct afhash *))0) {
                dsyslog(LOG_INFO,
                        "route in unsupported address family (%d), from %s (af %d)\n",
                        n->sa_family,
                        (*afswitch[from->sa_family].af_format)(from,
                                                               buf1, sizeof(buf1)),
                        from->sa_family);
                continue;
            }
            ptemp = (struct sockaddr_in *)&rip_dest;
            memset(ptemp,0x00,sizeof(struct sockaddr));
            ptemp->sin_family = n->sa_family;
            ptemp->sin_addr.s_addr = n->ip_addr;
            
            if (((*afp->af_checkhost)(&rip_dest)) == 0) {
                dsyslog(LOG_DEBUG,
                        "bad host %s in route from %s (af %d)\n",
                        (*afswitch[n->sa_family].af_format)(
                                                            &rip_dest, buf1, sizeof(buf1)),
                        (*afswitch[from->sa_family].af_format)(from,
                                                               buf2, sizeof(buf2)),
                        from->sa_family);
                continue;
			}
			if (n->rip_metric == 0){ /*receive n->rip_metric=16 packet*/
			//    (unsigned) n->rip_metric > HOPCNT_INFINITY) {
				if (memcmp(from, &badfrom2,
                           sizeof(badfrom2)) != 0) {
                    dsyslog(LOG_ERR,
                            "bad metric (%d) from %s\n",
                            n->rip_metric,
                            (*afswitch[from->sa_family].af_format)(from,
                                                                   buf1, sizeof(buf1)));
                    badfrom2 = *from;
                }
                continue;
            }
            /*
             * Adjust metric according to incoming interface.
             */
            if ((unsigned) n->rip_metric < HOPCNT_INFINITY)
                n->rip_metric += ifp->int_metric;
            if ((unsigned) n->rip_metric > HOPCNT_INFINITY)
                n->rip_metric = HOPCNT_INFINITY;
            n->subnetmask = ntohl(n->subnetmask);
            rt = rtlookup(&rip_dest, (struct in_addr *)&n->subnetmask, n->route_tag);
            if (rt == 0 ||
                (rt->rt_state & (RTS_INTERNAL|RTS_INTERFACE)) ==
                (RTS_INTERNAL|RTS_INTERFACE)) {
                /*
                 * If we're hearing a logical network route
                 * back from a peer to which we sent it,
                 * ignore it.
                 */
                if (rt && rt->rt_state & RTS_SUBNET &&
                    (*afp->af_sendroute)(rt, from))
                    continue;
                if ((unsigned)n->rip_metric < HOPCNT_INFINITY) {
                    /*
                     * Look for an equivalent route that
                     * includes this one before adding
                     * this route.
                     */
                    rt = rtfind(&rip_dest, (struct in_addr *)&n->subnetmask, n->route_tag);
                    if(rt!=0)
                    {
					    if(n->route_tag!=rt->rt_tag)
					    {
						    rt->rt_tag=n->route_tag;
					    }
					}
                    if (rt && equal(from, &rt->rt_router) && equal(&rip_dest,&rt->rt_dst) )
                        continue;
                        
                    struct sockaddr_in temp_hop;
                    temp_hop.sin_addr.s_addr = n->nexthop_ip;
                    memset(next_hop,0,20);
                    strncpy(next_hop,inet_ntoa(temp_hop.sin_addr),strlen(inet_ntoa(temp_hop.sin_addr)));
                    
                    rtadd(&rip_dest, from, n->rip_metric, 0,(struct in_addr *)&n->subnetmask,n->route_tag);
                    changes++;
                }
                continue;
            }
            
            /*
             * Update if from gateway and different,
             * shorter, or equivalent but old route
             * is getting stale.
             */
            if (equal(from, &rt->rt_router)) {
				if (n->rip_metric != rt->rt_metric) {
					/*if receive the same route but metirc>=16,del this route*/
					if(n->rip_metric >= HOPCNT_INFINITY){
						    rt = rt->rt_back;
				        rtdelete(rt->rt_forw);
				         continue;
						}
                    rtchange(rt, from, n->rip_metric,(struct in_addr *)&n->subnetmask,n->route_tag);
                    changes++;
                    rt->rt_timer = 0;
                    if (rt->rt_metric >= HOPCNT_INFINITY)
                        rt->rt_timer =
                            GARBAGE_TIME - EXPIRE_TIME;
                } else if (rt->rt_metric < HOPCNT_INFINITY)
                    rt->rt_timer = 0;
            } else if ((unsigned) n->rip_metric < (unsigned)rt->rt_metric ||
                       (rt->rt_metric == n->rip_metric &&
                        rt->rt_timer > (EXPIRE_TIME/2) &&
                        (unsigned) n->rip_metric < HOPCNT_INFINITY)) {
                /*!!!*/n->subnetmask = ntohl(n->subnetmask);
                rtchange(rt, from, n->rip_metric,(struct in_addr *)&n->subnetmask,n->route_tag);
                changes++;
                rt->rt_timer = 0;
            }
        }
        break;
    }
    /*
     * If changes have occurred, and if we have not sent a broadcast
     * recently, send a dynamic update.  This update is sent only
     * on interfaces other than the one on which we received notice
     * of the change.  If we are within MIN_WAITTIME of a full update,
     * don't bother sending; if we just sent a dynamic update
     * and set a timer (nextbcast), delay until that time.
     * If we just sent a full update, delay the dynamic update.
     * Set a timer for a randomized value to suppress additional
     * dynamic updates until it expires; if we delayed sending
     * the current changes, set needupdate.
     */
    if (changes && supplier &&
        now.tv_sec - lastfullupdate.tv_sec < SUPPLY_INTERVAL-MAX_WAITTIME) {
        u_long delay;
        
        if (now.tv_sec - lastbcast.tv_sec >= MIN_WAITTIME &&
            timercmp(&nextbcast, &now, <)) {
			//if (traceactions){/*do trigger update*/
                toall(supply, RTS_CHANGED, ifp);
		//}
            lastbcast = now;
            needupdate = 0;
            nextbcast.tv_sec = 0;
        } else {
            needupdate++;
            if (traceactions)
                fprintf(ftrace, "delay dynamic update\n");
        }
#define RANDOMDELAY()	(MIN_WAITTIME * 1000000 + \
		(u_long)random() % ((MAX_WAITTIME - MIN_WAITTIME) * 1000000))

        if (nextbcast.tv_sec == 0) {
            delay = RANDOMDELAY();
            if (traceactions)
                fprintf(ftrace,
                        "inhibit dynamic update for %ld usec\n",
                        delay);
            nextbcast.tv_sec = delay / 1000000;
            nextbcast.tv_usec = delay % 1000000;
            timevaladd(&nextbcast, &now);
            /*
             * If the next possibly dynamic update
             * is within MIN_WAITTIME of the next full update,
             * force the delay past the full update,
             * or we might send a dynamic update just before
             * the full update.
             */
            if (nextbcast.tv_sec > lastfullupdate.tv_sec +
                SUPPLY_INTERVAL - MIN_WAITTIME)
                nextbcast.tv_sec = lastfullupdate.tv_sec +
                    SUPPLY_INTERVAL + 1;
        }
    }
}


