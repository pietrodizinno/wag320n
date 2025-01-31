// $Id: logging.h,v 1.1.1.1 2009-01-05 09:01:18 fred_fu Exp $    --*- c++ -*--

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

#ifndef DHCP_FORWARDER_LOGGING_H
#define DHCP_FORWARDER_LOGGING_H

#include <netinet/in.h>

#include "output.h"

#define LOG(MSG)	writeMsg(MSG, sizeof(MSG)-1)
#define LOGSTR(MSG)	writeMsg(MSG, strlen(MSG))

#ifdef ENABLE_LOGGING
void logDHCPPackage(/*@in@*/char const *buffer, size_t len,
		    /*@in@*/struct in_pktinfo const	*pkinfo,
		    /*@in@*/void const			*addr)
  /*@globals internalState@*/
  /*@modifies internalState@*/ ;
#endif

#endif	//  DHCP_FORWARDER_LOGGING_H

  // Local Variables:
  // compile-command: "make -C .. -k"
  // fill-column: 80
  // End:
