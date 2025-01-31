// Copyright (C) 2002 Enrico Scholz <enrico.scholz@informatik.tu-chemnitz.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "splint_compat.h"

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/select.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <alloca.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <signal.h>

#include "if_packet.h"

#include "cfg.h"
#include "wrappers.h"
#include "dhcp.h"
#include "inet.h"
#include "recvfromflags.h"

#include "assertions.h"
#include "logging.h"
#include "debug.h"

#define CFG_LOGFILE		    "/var/log/dhcp-fwd.log"
  //#define ENABLE_AGENT_REPLACE		1

int sno;

typedef enum {
  acIGNORE,		//< Do nothing...
  acREMOVE_ID,		//< Remove an already existing agent-id field
  acADD_ID		//< Add an agent-id field if such a field does not
			//< already exists
#ifdef ENABLE_AGENT_REPLACE
  ,acREPLACE_ID		//< Replace an already existing agent-id field
#endif
} OptionFillAction;

/*@checkmod@*/static struct ServerInfoList	servers;
/*@checkmod@*/static struct FdInfoList	fds;

uint32_t  getDword(void *p)
{
    uint32_t  *tmp_pointer=(uint32_t  *)((int )p&0xfffffffc);
    int  tmp_index=(int) ((int )p&0x3)<<3;
    if(!tmp_index)
    	return  *tmp_pointer;
    return ((*tmp_pointer)>>tmp_index)|((*(++tmp_pointer))<<(32-tmp_index));
}

inline static void
fillFDSet(/*@out@*/fd_set			*fdset,
	  /*@out@*/int				*max)
    /*@globals fds@*/
    /*@modifies *fdset, *max@*/
{
    /*@-nullptrarith@*/
  struct FdInfo const *		fdinfo;
  struct FdInfo const * const	end_fdinfo = fds.dta+fds.len;
    /*@=nullptrarith@*/

  assert(fds.dta!=0 || fds.len==0);

  FD_ZERO(fdset);
  *max = -1;

  for (fdinfo=fds.dta; fdinfo<end_fdinfo; ++fdinfo) {
    assert(fdinfo!=0);
    *max = MAX(*max, fdinfo->fd);
    FD_SET(fdinfo->fd, fdset);
  }
}

inline static /*@exposed@*//*@null@*/struct FdInfo const *
lookupFD(/*@in@*/struct in_addr const addr)
    /*@globals fds@*/
{
    /*@-nullptrarith@*/
  struct FdInfo const *			fdinfo;
  struct FdInfo const * const		end_fdinfo = fds.dta+fds.len;
    /*@=nullptrarith@*/

  assert(fds.dta!=0 || fds.len==0);

  for (fdinfo=fds.dta; fdinfo<end_fdinfo; ++fdinfo) {
    assert(fdinfo!=0);

      /* We must check for both the real and "faked" giaddr IP here */
    if (fdinfo->iface->if_real_ip==addr.s_addr ||
	fdinfo->iface->if_ip == addr.s_addr) return fdinfo;
  }

  return 0;
}

inline static size_t
determineMaxMTU()
    /*@globals fds@*/
    /*@modifies@*/
{
  size_t				result = 1500;
    /*@-nullptrarith@*/
  struct FdInfo const *			fdinfo;
  struct FdInfo const * const		end_fdinfo = fds.dta+fds.len;
    /*@=nullptrarith@*/

  assert(fds.dta!=0 || fds.len==0);

  for (fdinfo=fds.dta; fdinfo<end_fdinfo; ++fdinfo) {
    assert(fdinfo!=0);

    result = MAX(result, fdinfo->iface->if_mtu);
  }

  return result;
}

inline static bool
isValidHeader(/*@in@*/struct DHCPHeader *header)
    /*@globals internalState@*/
    /*@modifies internalState@*/
{
  char const		*reason = 0;

    /*@-strictops@*/
  if ((header->flags&~flgDHCP_BCAST)!=0) { reason = "Invalid flags field"; }
    /*@=strictops@*/
  else if (header->hops>=MAX_HOPS)       { reason = "Looping detected"; }
#if 0
  else switch (header->htype) {
    case ARPHRD_ETHER	:
      if (header->hlen!=ETH_ALEN) {  reason = "Invalid hlen for ethernet"; }
      break;
    default		:
      break;	// Not active handled by us; will be forwarded as-is
  }
#else
  else {}
#endif

  if (reason==0) switch (header->op) {
    case opBOOTREPLY	:
    case opBOOTREQUEST	:  break;
    default		:  reason = "Unknown operation"; break;
  };

  if (reason!=0) LOGSTR(reason);

  return reason==0;
}

inline static bool
isValidOptions(/*@in@*/struct DHCPOptions const	*options,
	       size_t				o_len)
    /*@*/
{
  bool				seen_end     = false;
  struct DHCPSingleOption const	*opt         = reinterpret_cast(struct DHCPSingleOption const *)(options->data);
  struct DHCPSingleOption const	*end_options = reinterpret_cast(struct DHCPSingleOption const *)(&reinterpret_cast(uint8_t const *)(options)[o_len]);

    /* Is this really ok? Is an empty option-field RFC compliant? */
  if (o_len==0) return true;
  if (o_len<=4) return false;
  if (options->cookie != optDHCP_COOKIE) return false;

  do {
    switch (opt->code) {
      case cdEND	:  seen_end = true; break;
      default		:  break;
    }

    opt = DHCP_nextSingleOptionConst(opt);
  } while (opt < end_options);

  return (seen_end && opt==end_options);
}

inline static size_t
addAgentOption(/*@in@*/struct InterfaceInfo const * const	iface,
	       struct DHCPSingleOption				*end_opt,
	       size_t						len)
    /*@modifies *end_opt@*/
{
    /* Replace the end-tag

     *  - - - - - - -----
     * |           | END |
     *  - - - - - - -----
     * with
     *  - - - - - - ----- ----- ----- ----- ------------ -----
     * |           | 82  | len | sub | sub | ... id ... | END |
     * |           |     |     | opt | len |            |     |
     *  - - - - - - ----- ----- ----- ----- ------------ -----
     * */

  assert(strlen(iface->aid)<=IFNAMSIZ);

    /* Add space needed for our RFC 3046 agent id. See figure above for
     * details. */
  len += 4 + strlen(iface->aid);

    /* 'len' should now have the length of the complete option-field. RFC 2131
     * sets a lower limit of 312 octets, so we are checking against this
     * value. Since the function got only the real options without the
     * magic-cookie, 4 octets must be added.
     *
     * Because the underlying buffer was declared to hold more than this
     * minimum amount, we can exclude overflows here.
     *
     * Further versions of this software should make it possible to configure
     * the maximum size at runtime. */
  if (len+4 < 312) {
      /* replace old end-tag with our information */
    end_opt->code    = cdRELAY_AGENT;
    end_opt->data[0] = agCIRCUITID;	/* circuit id code as specified by RFC 3046 */
    end_opt->data[1] = static_cast(uint8_t)(strlen(iface->aid));
    end_opt->len     = 2 + end_opt->data[1];
    memcpy(&end_opt->data[2], iface->aid, end_opt->data[1]);

      /* set new end-tag */
    end_opt = DHCP_nextSingleOption(end_opt);
    end_opt->code = cdEND;
  }

  return len;
}

#ifdef ENABLE_AGENT_REPLACE
inline static size_t
replaceAgentOption(/*@in@*/struct InterfaceInfo const * const	iface,
		   struct DHCPSingleOption			*relay_opt,
		   struct DHCPSingleOption			*end_opt,
		   size_t					len)
    /*@requires end_opt > relay_opt@*/
{
  size_t	opt_len = DHCP_getOptionLength(relay_opt);
  size_t	str_len = strlen(iface->aid);

  assert(relay_opt!=0);

  if (str_len+4<=opt_len) {
    relay_opt->len     = str_len + 2;
    relay_opt->data[1] = static_cast(uint8_t)(str_len);
    memcpy(relay_opt->data+2, iface->aid, str_len);

    if (str_len+4<opt_len)
	// TODO: move memory; do not pad to satisfy WinNT
      memset(relay_opt->data+2+str_len, /*@+charint@*/cdPAD/*@=charint@*/, opt_len-str_len-4);
  }
  else if (opt_len>=len) {
    DHCP_removeOption(relay_opt, end_opt);

    len -= opt_len;
    len  = addAgentOption(iface, end_opt, len);
  }
  else {
    LOGSTR("Failed assertion 'opt_len < len'");
  }

  return len;
}
#endif

inline static size_t
removeAgentOption(/*@dependent@*/struct DHCPSingleOption	*opt,
		  struct DHCPSingleOption const 		*end_opt,
		  size_t					len)
    /*@requires end_opt > opt@*/
    /*@modifies *opt@*/
{
  size_t	opt_len = DHCP_getOptionLength(opt);

  if (opt_len < len) {
    DHCP_removeOption(opt, end_opt);
    len -= opt_len;
  }
  else LOGSTR("Failed assertion 'opt_len < len'");

  return len;
}

  /*@-mustmod@*/
inline static size_t
fillOptions(/*@in@*/struct InterfaceInfo const* const	iface,
	    /*@dependent@*/void				*option_ptr,
	    OptionFillAction				action)
    /*@modifies *option_ptr@*/
{
  /*@dependent@*/struct DHCPSingleOption 	*opt       = static_cast(struct DHCPSingleOption *)(option_ptr);
  /*@dependent@*/struct DHCPSingleOption	*end_opt   = 0;
  /*@dependent@*/struct DHCPSingleOption	*relay_opt = 0;
  size_t			len;

  do {
    switch (opt->code) {
      case cdRELAY_AGENT	:
	if (opt->data[0]==agCIRCUITID) relay_opt = opt;
	break;
      case cdEND		:  end_opt   = opt; break;
      default			:  break;
    }

    opt = DHCP_nextSingleOption(opt);
  } while (end_opt==0);

  assert(static_cast(void*)(end_opt)>=option_ptr);
  assert(end_opt>=relay_opt || relay_opt==0);

    /* Determine used space until end-tag and add space for the end-tag itself
     * (1 octet). */
    /*@-strictops@*/
  len  = (reinterpret_cast(char *)(end_opt) -
	  static_cast(char *)(option_ptr) + 1u);
    /*@=strictops@*/

  switch (action) {
    case acREMOVE_ID	:
      if (relay_opt!=0) len = removeAgentOption(relay_opt, end_opt, len);
      break;
    case acADD_ID	:
      if (relay_opt==0) len = addAgentOption(iface, end_opt, len);
      break;
#ifdef ENABLE_AGENT_REPLACE
    case acREPLACE_ID	:
      if (relay_opt==0) len = addOption(end_opt, len);
      else              len = replaceAgentOption(iface, relay_opt, end_opt, len);
      break;
#endif
    case acIGNORE	:  break;
    default		:  assert(false);
  }

  return len;
}
  /*@=mustmod@*/

inline static uint16_t
calculateCheckSum(/*@in@*/void const * const	dta,
		  size_t size,
		  uint32_t sum)
    /*@*/
{
  size_t		i;
  uint16_t const	*data = reinterpret_cast(uint16_t const *)(dta);

  for (i=0; i<size/2; ++i) sum += ntohs(data[i]);

  if (size%2 != 0) {
    union {
	uint8_t		aval[2];
	uint16_t	ival;
    } end_data;

    end_data.ival    = 0;
    end_data.aval[0] = reinterpret_cast(uint8_t const *)(data)[size-1];
    end_data.aval[1] = 0;
    sum += ntohs(end_data.ival);
  }

  while ( (sum>>16)!=0 )
    sum = (sum & 0xFFFF) + (sum >> 16);

  return sum;
}

inline static void
fixCheckSumIP(struct iphdr * const	ip)
    /*@modifies *ip@*/
{
  ip->check = 0;
  ip->check = htons(~calculateCheckSum(ip, sizeof(*ip), 0));
}

static void
fixCheckSumUDP(struct udphdr * const			udp,
	       /*@in@*/struct iphdr const * const	ip,
	       /*@in@*/void const * const		data)
    /*@modifies *udp@*/
{
  uint32_t		sum;
  struct {
      uint32_t		src;
      uint32_t		dst;
      uint8_t		mbz;
      uint8_t		proto;
      uint16_t		len;
  } __attribute__((__packed__))	pseudo_hdr;

  pseudo_hdr.src   = ip->saddr;
  pseudo_hdr.dst   = ip->daddr;
  pseudo_hdr.mbz   = 0;
  pseudo_hdr.proto = ip->protocol;
  pseudo_hdr.len   = udp->len;

  udp->check = 0;
  sum = calculateCheckSum(&pseudo_hdr, sizeof pseudo_hdr, 0);
  sum = calculateCheckSum(udp,         sizeof(*udp),      sum);
  sum = calculateCheckSum(data,        ntohs(udp->len)-sizeof(*udp), sum);

  sum = ~ntohs(sum);
  if (sum==0) sum=~sum;
  udp->check = sum;
}

inline static void
sendEtherFrame(/*@in@*/struct InterfaceInfo const	*iface,
	       /*@dependent@*/struct DHCPllPacket	*frame,
	       /*@dependent@*//*@in@*/char const	*buffer,
	       size_t					size)
    /*@globals internalState, fds@*/
    /*@modifies internalState, *frame@*/
{
  struct sockaddr_ll		sock;
  struct msghdr			msg;
  /*@temp@*/struct iovec	iovec_data[2];
    /*@-sizeoftype@*/
  size_t const			szUDPHDR = sizeof(struct udphdr);
    /*@=sizeoftype@*/

    /* We support ethernet only and the config-part shall return ethernet-macs
     * only... */
  assert(iface->if_maclen == sizeof(frame->eth.ether_dhost));

  memset(&sock, 0, sizeof sock);
  sock.sll_family    = static_cast(sa_family_t)(AF_PACKET);
  sock.sll_ifindex   = static_cast(int)(iface->if_idx);
    /* We do not need to initialize the other attributes of rcpt_sock since
     * dst-hwaddr et.al. are determined by the ethernet-frame defined below */

  memcpy(frame->eth.ether_shost, iface->if_mac,  iface->if_maclen);

  frame->ip.version  = 4u;
  frame->ip.ihl      = sizeof(frame->ip)/4u;
  frame->ip.tos      = 0;
  frame->ip.tot_len  = htons(sizeof(frame->ip) + szUDPHDR + size);
  frame->ip.id       = 0;
  frame->ip.frag_off = htons(IP_DF);
  frame->ip.ttl      = 64;
  frame->ip.protocol = IPPROTO_UDP;
  frame->ip.saddr    = iface->if_ip;

  frame->udp.len     = htons(szUDPHDR + size);

  fixCheckSumIP(&frame->ip);
  fixCheckSumUDP(&frame->udp, &frame->ip, buffer);

  iovec_data[0].iov_base = frame;
  iovec_data[0].iov_len  = sizeof(*frame);
  iovec_data[1].iov_base = const_cast(char *)(buffer);
  iovec_data[1].iov_len  = size;

  msg.msg_name       = &sock;
  msg.msg_namelen    = sizeof(sock);
  msg.msg_iov        = iovec_data;
  msg.msg_iovlen     = 2;
  msg.msg_control    = 0;
  msg.msg_controllen = 0;
  msg.msg_flags      = 0;

  assertDefined(msg.msg_iov);

  Wsendmsg(fds.raw_fd, &msg, 0);
}

inline static void
sendToClient(/*@in@*/struct FdInfo const * const	fd,
	     /*@in@*/struct DHCPHeader const * const	header,
	     /*@in@*//*@dependent@*/char const * const	buffer,
	     size_t const				size)
    /*@globals internalState, fds@*/
    /*@modifies internalState@*/
{
  struct DHCPllPacket		frame;
  struct InterfaceInfo const	*iface = fd->iface;

  assert(header->op   == opBOOTREPLY);

  memset(&frame, 0, sizeof frame);
  frame.eth.ether_type = htons(ETHERTYPE_IP);

    /* Check whether header contains an ethernet MAC or something else (e.g. a
     * PPP tag). In the first case send to this MAC, in the latter one, send a
     * ethernet-broadcast message */
  if (header->htype==ARPHRD_ETHER && header->hlen==ETH_ALEN)
    memcpy(frame.eth.ether_dhost, header->chaddr, sizeof frame.eth.ether_dhost);
  else
    memset(frame.eth.ether_dhost, 255,            sizeof frame.eth.ether_dhost);

  if ((header->flags&flgDHCP_BCAST)!=0 && header->ciaddr!=0)
    frame.ip.daddr  = header->ciaddr;
  else if (iface->allow_bcast)
    frame.ip.daddr  = INADDR_BROADCAST;
  else
    return;	//< \todo

  frame.udp.source  = htons(DHCP_PORT_SERVER);
  frame.udp.dest    = htons(DHCP_PORT_CLIENT);

  sendEtherFrame(iface, &frame, buffer, size);
}

inline static void
sendServerBcast(/*@in@*/struct ServerInfo const	* const		server,
		/*@dependent@*//*@in@*/char const * const	buffer,
		size_t const					size)
    /*@globals internalState, fds@*/
    /*@modifies internalState@*/
{
  struct DHCPllPacket		frame;
  struct InterfaceInfo const	*iface = server->info.iface;

  memset(&frame, 0, sizeof frame);
  memset(frame.eth.ether_dhost, 255, sizeof frame.eth.ether_dhost);

  frame.eth.ether_type = htons(ETHERTYPE_IP);

  frame.ip.daddr       = INADDR_BROADCAST;

  frame.udp.source     = htons(DHCP_PORT_CLIENT);
  frame.udp.dest       = htons(DHCP_PORT_SERVER);
  sendEtherFrame(iface, &frame, buffer, size);
}

inline static void
sendServerUnicast(/*@in@*/struct ServerInfo const * const	server,
		  /*@in@*/char const * const			buffer,
		  size_t const					size)
    /*@globals internalState, fds@*/
    /*@modifies internalState@*/
{
  struct sockaddr_in	sock;

  memset(&sock, 0, sizeof sock);
  sock.sin_family = AF_INET;

  sock.sin_addr = server->info.ip;
  sock.sin_port = htons(DHCP_PORT_SERVER);
  Wsendto(fds.sender_fd, buffer, size, 0,
	  reinterpret_cast(struct sockaddr *)(&sock),
	  sizeof sock);
}

inline static void
sendToServer(/*@in@*//*@dependent@*/char const * const	buffer,
	     size_t const				size)
    /*@globals servers, fds, internalState@*/
    /*@modifies internalState@*/
{
    /*@-nullptrarith@*/
  struct ServerInfo const *		server;
  struct ServerInfo const * const	end_server = servers.dta+servers.len;
    /*@=nullptrarith@*/

  assert(servers.len==0 || servers.dta!=0);

  for (server=servers.dta; server<end_server; ++server) {
    assert(server!=0);

    switch (server->type) {
      case svUNICAST	:
	sendServerUnicast(server, buffer, size);
	break;
      case svBCAST	:
	sendServerBcast(server, buffer, size);
	break;
      default		:
	assert(false);
    }
  }
}

inline static void
handlePacket(/*@in@*/struct FdInfo const * const		fd,
	     /*@in@*/struct InterfaceInfo const * const		iface_orig,
	     /*@dependent@*/char * const			buffer,
	     size_t						size)
    /*@globals fds, servers, internalState@*/
    /*@modifies internalState@*/
{
  struct DHCPHeader * const	header  = reinterpret_cast(struct DHCPHeader *)(buffer);
  struct DHCPOptions * const	options = reinterpret_cast(struct DHCPOptions *)(&buffer[sizeof(*header)]);
  size_t			options_len = size - sizeof(*header);


    /* Discard broken header (e.g. too much hops or bad values) */
  if (!isValidHeader(header)){
     return;
    }
  ++header->hops;

    /* Check if we are the first agent or if we handle a packet indented for us;
     * if so, set 'giaddr' and the relay-agent field, else do not touch the
     * packet */
  if (header->giaddr==0 || header->giaddr==fd->iface->if_ip) {
    header->giaddr = fd->iface->if_ip;

    if (!isValidOptions(options, options_len)) {
      LOG("Invalid options");
      return;
    }

      /* Do not fill the option-field if this field does not exists. */
    if (options_len!=0) {
      OptionFillAction		action;
      switch (header->op) {
	case opBOOTREPLY	:
	    action = acREMOVE_ID;
	    break;
	case opBOOTREQUEST	:
	    action = acADD_ID;
	    break;
	default			:
	    assert(false);
	    action = acIGNORE;
	    break;
      }

	/* Fill agent-info and adjust size-information */
      options_len  = fillOptions(fd->iface, options->data, action);
      options_len += sizeof(options->cookie);
      size         = options_len + sizeof(*header);
    }

    assert(isValidOptions(options, options_len));
  }

  switch (header->op) {
    case opBOOTREPLY	:
      if      (!iface_orig->has_servers){

        LOG("BOOTREPLY request from interface without servers");
    }
      else if (!fd->iface->has_clients){
        LOG("BOOTREPLY request for interface without clients");
    }
      else{
       sendToClient(fd, header, buffer, size);
    }
      break;
    case opBOOTREQUEST	:
      if (!iface_orig->has_clients){
         LOG("BOOTREQUEST request from interface without clients");
        }
      else{
      sendToServer(buffer, size);
    }
      break;

	/* isValidHeader() checked the correctness of header->op already and it
	 * should be impossible to reach this code... */
    default		:
        assert(false);
  }
}

/*@noreturn@*/
inline static void
execRelay()
    /*@globals fds, servers, internalState@*/
    /*@modifies internalState@*/
{
    /*@-sizeoftype@*/
  size_t const			szDHCPHDR = sizeof(struct DHCPHeader);
    /*@=sizeoftype@*/
  size_t const			max_mtu   = determineMaxMTU();
  size_t const			len_total = max_mtu + IFNAMSIZ + 4;
  char				*buffer   = static_cast(char *)(alloca(len_total));

  FatalErrnoError(buffer==0, 1, "alloca()");

  assert(fds.dta!=0 || fds.len==0);

  while (true) {
    fd_set			fdset;
    int				max;
      /*@-nullptrarith@*/
    struct FdInfo const *	fdinfo;
    struct FdInfo const * const end_fdinfo = fds.dta+fds.len;
      /*@=nullptrarith@*/

    fillFDSet(&fdset, &max);
    if /*@-type@*/(Wselect(max+1, &fdset, 0, 0, 0)==-1)/*@=type@*/ continue;


    for (fdinfo=fds.dta; fdinfo<end_fdinfo; ++fdinfo) {
      size_t				size;
      struct sockaddr_in		addr;
      struct in_pktinfo			pkinfo;
      int				flags = 0;

      assert(fdinfo!=0);

      if (!FD_ISSET(fdinfo->fd, &fdset)) /*@innercontinue@*/continue;


	/* Is this really correct? Can we receive fragmented IP datagrams being
	 * assembled larger than the MTU of the attached interfaces? */
      size = WrecvfromFlagsInet4(fdinfo->fd, buffer, len_total, &flags,
				 &addr, &pkinfo);

    debug("the port is %d, the ip is %s", addr.sin_port, inet_ntoa(addr.sin_addr));

#ifdef ENABLE_LOGGING
      logDHCPPackage(buffer, size, &pkinfo, &addr);
#endif

      if (static_cast(ssize_t)(size)==-1) {
	char const *	msg = strerror(errno);
	LOG("recvfrom() failed");
	LOGSTR(msg);
      }
      else if (size < szDHCPHDR) {
	LOG("Malformed package");
      }
      else {
	struct InterfaceInfo const * const	iface_orig = fdinfo->iface;
	struct FdInfo const *			fd_real    = fdinfo;

	  /* Broadcast messages are designated for the interface, so lookup the
	   * "real" dest-interface for unicast-messages only.
	   *
	   * \todo: Can we setup the routing that datagrams from the server are
	   *        received by the giaddr-interface? */
	if (pkinfo.ipi_addr.s_addr!=INADDR_BROADCAST)
	  fd_real = lookupFD(pkinfo.ipi_addr);
	if (fd_real==0){
	     LOG("Received package on unknown interface");
    }
	else if (size > fd_real->iface->if_mtu) {
	  LOG("Unexpected large packet");
	}
	else{
	  handlePacket(fd_real, iface_orig, buffer, size);
	}
      }
    }
  }
}

void die(int n)
{
    close(sno);
    unlink(CFG_LOGFILE);
    exit(n);
}

int
main(int argc, char *argv[])
    /*@globals fds, servers, internalState, fileSystem@*/
    /*@modifies fds, servers, internalState, fileSystem@*/
{
    struct InterfaceInfoList ifs;
    int ret=0;
   // FILE *fp_cfg=NULL;
   // pid_t pid;
   // struct sigaction sv;
   // unsigned char tmp_str[128];

    debug("Begin....\n");
    checkCompileTimeAssertions();

#if 1 //ada
    /* We create a mem-leak here, but this is not important because the parent
     * exits immediately and frees the resources and the child never exits */
    /*@-compdestroy@*//*@-superuser@*/
    ret = initializeSystem(argc, argv, &ifs, &servers, &fds);

    switch (ret) {
        /*@=superuser@*/
        case -1	:  return 5;
        case 0	:  execRelay();
        default	:  return 0;
    }
    /*@=compdestroy@*/
#else

    /** judge the daemon is already (the file exist or not ) runing or not **/
    fp_cfg = fopen(CFG_LOGFILE, "r");
    if (fp_cfg != NULL) {
        if (fgets(tmp_str, sizeof(tmp_str), fp_cfg) && !kill(pid = atol(tmp_str), 0)) {
            fclose(fp_cfg);
            return -1;
        }
        fclose(fp_cfg);
    }

    sv.sa_handler = die;
    sigemptyset( &sv.sa_mask);
    sv.sa_flags = 0;
    if ( sigaction(SIGTERM, &sv, 0) < 0 ) {
        exit(1);
    }
    if ( (pid=fork() ) !=0 )
        exit(0);     /** parents terminates **/

    setsid();
    signal(SIGHUP, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    if ( (pid=fork() !=0) ){

        ret = initializeSystem(argc, argv, &ifs, &servers, &fds);
    debug("the ret is %d", ret);
        switch (ret) {
            /*@=superuser@*/
            case -1	:  return 5;
            case 0	:  execRelay();
            default	:  return 0;
        }
        exit(0);   /** first child terminates **/
    }
    chdir("/");
    umask(0);

    if ((fp_cfg = fopen(CFG_LOGFILE, "w")) == NULL) {
        return -1;
    }
    fprintf(fp_cfg, "%d\n", getpid()); /** record pid to lock file **/
    fclose(fp_cfg);
    return 0;
#endif
}

  // Local Variables:
  // compile-command: "make -C .. -k"
  // fill-column: 80
  // End:
