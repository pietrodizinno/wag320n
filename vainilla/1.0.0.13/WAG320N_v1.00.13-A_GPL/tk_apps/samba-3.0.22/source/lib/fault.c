/* 
   Unix SMB/CIFS implementation.
   Critical Fault handling
   Copyright (C) Andrew Tridgell 1992-1998
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "includes.h"

static void (*cont_fn)(void *);

/*******************************************************************
report a fault
********************************************************************/
static void fault_report(int sig)
{
	static int counter;

	if (counter) _exit(1);

	counter++;

	DEBUG(0,("===============================================================\n"));
	DEBUG(0,("INTERNAL ERROR: Signal %d in pid %d (%s)",sig,(int)sys_getpid(),SAMBA_VERSION_STRING));
	DEBUG(0,("\nPlease read the Trouble-Shooting section of the Samba3-HOWTO\n"));
	DEBUG(0,("\nFrom: http://www.samba.org/samba/docs/Samba3-HOWTO.pdf\n"));
	DEBUG(0,("===============================================================\n"));
  
	smb_panic("internal error");

	if (cont_fn) {
		cont_fn(NULL);
#ifdef SIGSEGV
		CatchSignal(SIGSEGV,SIGNAL_CAST SIG_DFL);
#endif
#ifdef SIGBUS
		CatchSignal(SIGBUS,SIGNAL_CAST SIG_DFL);
#endif
#ifdef SIGABRT
		CatchSignal(SIGABRT,SIGNAL_CAST SIG_DFL);
#endif
		return; /* this should cause a core dump */
	}
	exit(1);
}

/****************************************************************************
catch serious errors
****************************************************************************/
static void sig_fault(int sig)
{
	fault_report(sig);
}

/*******************************************************************
setup our fault handlers
********************************************************************/
void fault_setup(void (*fn)(void *))
{
	cont_fn = fn;

#ifdef SIGSEGV
	CatchSignal(SIGSEGV,SIGNAL_CAST sig_fault);
#endif
#ifdef SIGBUS
	CatchSignal(SIGBUS,SIGNAL_CAST sig_fault);
#endif
#ifdef SIGABRT
	CatchSignal(SIGABRT,SIGNAL_CAST sig_fault);
#endif
}
