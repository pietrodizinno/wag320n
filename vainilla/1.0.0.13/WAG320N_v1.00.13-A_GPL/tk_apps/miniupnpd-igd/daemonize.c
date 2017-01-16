/* $Id: daemonize.c,v 1.1.1.1 2009-01-05 09:01:14 fred_fu Exp $ */
/* MiniUPnP project
 * http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/
 * (c) 2006 Thomas Bernard 
 * This software is subject to the conditions detailed
 * in the LICENCE file provided within the distribution */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>

#include "daemonize.h"

int
daemonize(void)
{
	int pid, i;

	switch(fork())
	{
	/* fork error */
	case -1:
		perror("fork()");
		exit(1);
	
	/* child process */
	case 0:
		/* obtain a new process group */
		if( (pid = setsid()) < 0)
		{
			perror("setsid()");
			exit(1);
		}

		/* close all descriptors */
//		for (i=getdtablesize();i>=0;--i) close(i);		

		i = open("/dev/null",O_RDWR); /* open stdin */
		dup(i); /* stdout */
		dup(i); /* stderr */
		umask(027);
		chdir("/"); /* chdir to /tmp ? */			

		close(i);
		return pid;

	/* parent process */
	default:
		exit(0);
	}
}

int
writepidfile(const char * fname, int pid)
{
	char * pidstring;
	int pidstringlen;
	int pidfile;

	if(!fname || (strlen(fname) == 0))
		return -1;
	
	if( (pidfile = open(fname, O_WRONLY|O_CREAT, 0666)) < 0)
	{
		syslog(LOG_ERR, "Unable to write to pidfile %s: %m", fname);
		return -1;
	}

	pidstringlen = asprintf(&pidstring, "%d\n", pid);
	if(pidstringlen < 0)
	{
		syslog(LOG_ERR, 
			"Unable to write to pidfile %s: asprintf(): FAILED", fname);
		close(pidfile);
		return -1;
	}
	else
	{
		write(pidfile, pidstring, pidstringlen);
		free(pidstring);
	}

	close(pidfile);

	return 0;
}

int
checkforrunning(const char * fname)
{
	char buffer[64];
	int pidfile;
	pid_t pid;

	if(!fname || (strlen(fname) == 0))
		return -1;

	if( (pidfile = open(fname, O_RDONLY)) < 0)
		return 0;
	
	memset(buffer, 0, 64);
	
	if(read(pidfile, buffer, 63))
	{
		if( (pid = atol(buffer)) > 0)
		{
			if(!kill(pid, 0))
			{
				close(pidfile);
				return -2;
			}
		}
	}
	
	close(pidfile);
	
	return 0;
}

