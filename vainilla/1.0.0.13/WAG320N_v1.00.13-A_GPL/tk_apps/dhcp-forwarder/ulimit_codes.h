// $Id: ulimit_codes.h,v 1.1.1.1 2009-01-05 09:01:18 fred_fu Exp $    --*- c++ -*--

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

#ifndef ensc_DHCP_FORWARDER_ULIMIT_H_I_KNOW_WHAT_I_DO
#  error ulimit_codes.h can not be used in this way
#endif

static
struct {
    /*@observer@*/
    char	*name;
    int		code;
} const  ULIMIT_CODES[] =
{
  { "stack",   RLIMIT_STACK },
  { "data",    RLIMIT_DATA },
  { "core",    RLIMIT_CORE },
  { "rss",     RLIMIT_RSS },
  { "nproc",   RLIMIT_NPROC },
  { "nofile",  RLIMIT_NOFILE },
  { "memlock", RLIMIT_MEMLOCK },
  { "as",      RLIMIT_AS }
#ifdef RLIMIT_LOCKS    
  , { "locks",   RLIMIT_LOCKS }
#else
#  warning RLIMIT_LOCKS limit not set
#endif
};

#undef ensc_DHCP_FORWARDER_ULIMIT_H_I_KNOW_WHAT_I_DO
