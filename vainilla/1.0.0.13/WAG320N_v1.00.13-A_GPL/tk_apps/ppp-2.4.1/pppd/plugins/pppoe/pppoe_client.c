/* PPPoE support library "libpppoe"
 *
 * Copyright 2000 Michal Ostrowski <mostrows@styx.uwaterloo.ca>,
 *		  Jamal Hadi Salim <hadi@cyberus.ca>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 */
 
#include "pppoe.h"
#include "utils.h"

extern unsigned int host_uniq_index;
extern unsigned int host_uniq_padi;

extern char	*pppoe_srv_name;

char poesrv_mac[18]={0},pado_mac_tmp[18]={0};

int send_padt(struct session *ses,struct pppoe_packet *p_in){
    struct pppoe_packet padt;
    struct pppoe_hdr hdr;
    unsigned int host_uniq_tmp;

    memset(&padt,0,sizeof(struct pppoe_packet));
    memcpy(&padt.addr, &p_in->addr, sizeof(struct sockaddr_ll));
    memcpy(&hdr, (struct pppoe_hdr*) ses->curr_pkt.buf, sizeof(struct pppoe_hdr));

    padt.hdr = &hdr;
    padt.hdr->ver  = 1;
    padt.hdr->type = 1;
    padt.hdr->code = PADT_CODE;
    padt.hdr->sid  = __constant_htons(0);

    poe_info(ses,"Send PADT for unexpected PADO,ses->remote.sll_addr=%02x:%02x:%02x:%02x:%02x:%02x\n",
        ses->remote.sll_addr[0],ses->remote.sll_addr[1],
        ses->remote.sll_addr[2],ses->remote.sll_addr[3],
        ses->remote.sll_addr[4],ses->remote.sll_addr[5]);

    if(memcmp(p_in->addr.sll_addr,"\xff\xff\xff\xff\xff\xff",6)==0){
        poe_info(ses,"Don't send out broadcast PADT for unexpected PADO.\n");
        return 0;
    }   

    host_uniq_tmp = host_uniq_index;
    host_uniq_index = host_uniq_padi;
    send_disc(ses,&padt);
    host_uniq_index = host_uniq_tmp;

    return 0;
}

static int std_rcv_pado(struct session* ses,
			struct pppoe_packet *p_in,
			struct pppoe_packet **p_out)
{
    
    if( verify_packet(ses, p_in) < 0)
	return -1;
    if(ses->state != PADO_CODE ){
	poe_error(ses,"Unexpected packet: %P",p_in);

    sprintf(pado_mac_tmp,"%02x:%02x:%02x:%02x:%02x:%02x",
        p_in->addr.sll_addr[0],p_in->addr.sll_addr[1],
        p_in->addr.sll_addr[2],p_in->addr.sll_addr[3],
        p_in->addr.sll_addr[4],p_in->addr.sll_addr[5]);

    if(strcmp(poesrv_mac,pado_mac_tmp)) // send PADT for unselected PADOs
        send_padt(ses,p_in);

	return 0;
    }
    
    sprintf(poesrv_mac,"%02x:%02x:%02x:%02x:%02x:%02x",
        p_in->addr.sll_addr[0],p_in->addr.sll_addr[1],
        p_in->addr.sll_addr[2],p_in->addr.sll_addr[3],
        p_in->addr.sll_addr[4],p_in->addr.sll_addr[5]);

    host_uniq_index++;

    if (DEB_DISC2) {
	poe_dbglog (ses,"PADO received: %P", p_in);
    }
    
    memcpy(&ses->remote, &p_in->addr, sizeof(struct sockaddr_ll));
    memcpy( &ses->curr_pkt.addr, &ses->remote , sizeof(struct sockaddr_ll));
    
    ses->curr_pkt.hdr->code = PADR_CODE;
    
    /* The HOST_UNIQ has been verified already... there's no "if" about this */
    /* if(ses->filt->htag) */
    copy_tag(&ses->curr_pkt,get_tag(p_in->hdr,PTT_HOST_UNIQ));	
    
    if (ses->filt->ntag) {
    	ses->curr_pkt.tags[TAG_AC_NAME]=NULL;
    }
//    copy_tag(&ses->curr_pkt,get_tag(p_in->hdr,PTT_AC_NAME));
    
    if(ses->filt->stag) {
    	ses->curr_pkt.tags[TAG_SRV_NAME]=NULL;
    }
    copy_tag(&ses->curr_pkt,get_tag(p_in->hdr,PTT_SRV_NAME));
    
    copy_tag(&ses->curr_pkt,get_tag(p_in->hdr,PTT_AC_COOKIE));
    copy_tag(&ses->curr_pkt,get_tag(p_in->hdr,PTT_RELAY_SID));
    
    ses->state = PADS_CODE;
    
    ses->retransmits = 0;
    
    send_disc(ses, &ses->curr_pkt);
    (*p_out) = &ses->curr_pkt;
    if (ses->np)
	return 1;
    return 0;
}

static int std_init_disc(struct session* ses,
			 struct pppoe_packet *p_in,
			 struct pppoe_packet **p_out){
    srand(time(0));
    host_uniq_index = rand();
    host_uniq_padi = host_uniq_index;
    
    memset(&ses->curr_pkt,0, sizeof(struct pppoe_packet));

    
    /* Check if already connected */
    if( ses->state != PADO_CODE ){
	return 1;
    }
    
    ses->curr_pkt.hdr = (struct pppoe_hdr*) ses->curr_pkt.buf;
    ses->curr_pkt.hdr->ver  = 1;
    ses->curr_pkt.hdr->type = 1;
    ses->curr_pkt.hdr->code = PADI_CODE;
    
    
    memcpy( &ses->curr_pkt.addr, &ses->remote , sizeof(struct sockaddr_ll));
    
    poe_info (ses,"Sending PADI");
    if (DEB_DISC)
	poe_dbglog (ses,"Sending PADI");
    
    ses->retransmits = 0 ;
    
    if(ses->filt->ntag) {
	ses->curr_pkt.tags[TAG_AC_NAME]=ses->filt->ntag;
	poe_info(ses,"overriding AC name\n");
    }
    
    /* Ron fixed add pppoe srv support */ 
    if (pppoe_srv_name !=NULL) {
	if (strlen (pppoe_srv_name) > 255) {
	    poe_error (ses," Service name too long\
	                (maximum allowed 256 chars)");
	    poe_die(-1);
	}
	ses->filt->stag = make_filter_tag(PTT_SRV_NAME,
					  strlen(pppoe_srv_name),
					  pppoe_srv_name);
	if ( ses->filt->stag == NULL) {
	    poe_error (ses,"failed to malloc for service name");
	    poe_die(-1);
	}
	printf("pppoe_srv_name=%s\n",pppoe_srv_name);
    }
     
    if(ses->filt->stag)
	ses->curr_pkt.tags[TAG_SRV_NAME]=ses->filt->stag;
    else
    	printf("ses->filt->stag==NULL\n");

    if(ses->filt->htag) {
	ses->curr_pkt.tags[TAG_HOST_UNIQ] = ses->filt->htag;
    }
    
    ses->curr_pkt.hdr->code = PADT_CODE;
    send_disc(ses, &ses->curr_pkt);
    ses->curr_pkt.hdr->code = PADI_CODE;
    send_disc(ses, &ses->curr_pkt);
    (*p_out)= &ses->curr_pkt;
    return 0;
}


static int std_rcv_pads(struct session* ses,
			struct pppoe_packet *p_in,
			struct pppoe_packet **p_out)
{
    if( verify_packet(ses, p_in) < 0)
	return -1;
    
    if (DEB_DISC)
	poe_dbglog (ses,"Got connection: %x",
		    ntohs(p_in->hdr->sid));
    poe_info (ses,"Got connection: %x", ntohs(p_in->hdr->sid));
    
    ses->sp.sa_family = AF_PPPOX;
    ses->sp.sa_protocol = PX_PROTO_OE;
    ses->sp.sa_addr.pppoe.sid = p_in->hdr->sid;
    memcpy(ses->sp.sa_addr.pppoe.dev,ses->name, IFNAMSIZ);
    memcpy(ses->sp.sa_addr.pppoe.remote, p_in->addr.sll_addr, ETH_ALEN);
    
    return 1;
}

static int std_rcv_padt(struct session* ses,
			struct pppoe_packet *p_in,
			struct pppoe_packet **p_out)
{
    ses->state = PADO_CODE;
    return 0;
}


extern int disc_sock;
int client_init_ses (struct session *ses, char* devnam)
{
    int i=0;
    int retval;
    char dev[IFNAMSIZ+1];
    int addr[ETH_ALEN];
    int sid;
    
    /* do error checks here; session name etc are valid */
//    poe_info (ses,"init_ses: creating socket");
    
    /* Make socket if necessary */
    if( disc_sock < 0 )
    {	
		disc_sock = socket(PF_PACKET, SOCK_DGRAM, 0);
		if( disc_sock < 0 )
		{
		    poe_fatal(ses,
			      "Cannot create PF_PACKET socket for PPPoE discovery\n");
		}	
    }
    
    /* Check for long format */
    retval =sscanf(devnam, FMTSTRING(IFNAMSIZ),addr, addr+1, addr+2,
		   addr+3, addr+4, addr+5,&sid,dev);
    if( retval != 8 )
    {
		/* Verify the device name , construct ses->local */
		retval = get_sockaddr_ll(devnam,&ses->local);
		if (retval < 0)
		    poe_fatal(ses, "client_init_ses: "
			      "Cannot create PF_PACKET socket for PPPoE discovery\n");
		
		
		ses->state = PADO_CODE;
		memcpy(&ses->remote, &ses->local, sizeof(struct sockaddr_ll) );
		
		memset( ses->remote.sll_addr, 0xff, ETH_ALEN);
    }
    else
    {
		/* long form parsed */
	
		/* Verify the device name , construct ses->local */
		retval = get_sockaddr_ll(dev,&ses->local);
		if (retval < 0)
		    poe_fatal(ses,"client_init_ses(2): "
			      "Cannot create PF_PACKET socket for PPPoE discovery\n");
		ses->state = PADS_CODE;
		ses->sp.sa_family = AF_PPPOX;
		ses->sp.sa_protocol = PX_PROTO_OE;
		ses->sp.sa_addr.pppoe.sid = sid;
		
		memcpy(&ses->remote, &ses->local, sizeof(struct sockaddr_ll) );
		
		for(; i < ETH_ALEN ; ++i )
		{
		    ses->sp.sa_addr.pppoe.remote[i] = addr[i];
		    ses->remote.sll_addr[i]=addr[i];
		}
		memcpy(ses->sp.sa_addr.pppoe.dev, dev, IFNAMSIZ);
    }
    if( retval < 0 )
	{
		error("bad device name: %s",devnam);
	}   
    retval = bind( disc_sock ,
		   (struct sockaddr*)&ses->local,
		   sizeof(struct sockaddr_ll));   
    if( retval < 0 )
    {
		error("bind to PF_PACKET socket failed: %m");
    }
    
    ses->fd = socket(AF_PPPOX,SOCK_STREAM,PX_PROTO_OE);
    if(ses->fd < 0)
    {
		poe_fatal(ses,"Failed to create PPPoE socket: %m");
    }
    
    
    ses->init_disc = std_init_disc;
    ses->rcv_pado  = std_rcv_pado;
    ses->rcv_pads  = std_rcv_pads;
    ses->rcv_padt  = std_rcv_padt;
    
    /* this should be filter overridable */
#if 0
    /* Ron change for PADI retry */
    ses->retries = -1;
#else
    /* set retry > 7 will not pass cdrouter-16, modify to 7, then pass. */
    ses->retries = 7; 
#endif

    return ses->fd;
}

int client_init_ses_relay(struct session *ses, char* devnam)
{
    int i=0;
    int retval;
    char dev[IFNAMSIZ+1];
    int addr[ETH_ALEN];
    int sid;
    
    /* Make socket if necessary */
    if( disc_sock < 0 )
    {
		disc_sock = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_PPP_DISC));
		//disc_sock = socket(PF_PACKET, SOCK_DGRAM, 0);
		if( disc_sock < 0 )
		{
		    poe_fatal(ses,
			      "Cannot create PF_PACKET socket for PPPoE discovery\n");
		}
	
    }
    
    /* Check for long format */
    retval =sscanf(devnam, FMTSTRING(IFNAMSIZ),addr, addr+1, addr+2,
		   addr+3, addr+4, addr+5,&sid,dev);
    if( retval != 8 )
    {
		/* Verify the device name , construct ses->local */
		retval = get_sockaddr_ll(devnam,&ses->local);
		if (retval < 0)
		    poe_fatal(ses, "client_init_ses: "
			      "Cannot create PF_PACKET socket for PPPoE discovery\n");
		
		
		ses->state = PADO_CODE;
		memcpy(&ses->remote, &ses->local, sizeof(struct sockaddr_ll) );
		
		memset( ses->remote.sll_addr, 0xff, ETH_ALEN);
    }
    else
    {
		/* long form parsed */
	
		/* Verify the device name , construct ses->local */
		retval = get_sockaddr_ll(dev,&ses->local);
		if (retval < 0)
		    poe_fatal(ses,"client_init_ses(2): "
			      "Cannot create PF_PACKET socket for PPPoE discovery\n");
		ses->state = PADS_CODE;
		ses->sp.sa_family = AF_PPPOX;
		ses->sp.sa_protocol = PX_PROTO_OE;
		ses->sp.sa_addr.pppoe.sid = sid;
		
		memcpy(&ses->remote, &ses->local, sizeof(struct sockaddr_ll) );
		
		for(; i < ETH_ALEN ; ++i )
		{
		    ses->sp.sa_addr.pppoe.remote[i] = addr[i];
		    ses->remote.sll_addr[i]=addr[i];
		}
		memcpy(ses->sp.sa_addr.pppoe.dev, dev, IFNAMSIZ);
    }
    if( retval < 0 )
	error("bad device name: %s",devnam);
    
#if 0    
    retval = bind( disc_sock ,
		   (struct sockaddr*)&ses->local,
		   sizeof(struct sockaddr_ll));
    
    
    if( retval < 0 )
    {
		error("bind to PF_PACKET socket failed: %m");
    }
#endif    
    ses->fd = socket(AF_PPPOX,SOCK_STREAM,PX_PROTO_OE);
    if(ses->fd < 0)
    {
		poe_fatal(ses,"Failed to create PPPoE socket: %m");
    }
    
    
    ses->init_disc = std_init_disc;
    ses->rcv_pado  = std_rcv_pado;
    ses->rcv_pads  = std_rcv_pads;
    ses->rcv_padt  = std_rcv_padt;
    
    /* this should be filter overridable */
    ses->retries = 7;
    
    return ses->fd;
}
