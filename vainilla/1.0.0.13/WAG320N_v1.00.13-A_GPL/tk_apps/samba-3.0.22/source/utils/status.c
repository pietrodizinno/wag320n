/* 
   Unix SMB/CIFS implementation.
   status reporting
   Copyright (C) Andrew Tridgell 1994-1998
   
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

   Revision History:

   12 aug 96: Erik.Devriendt@te6.siemens.be
   added support for shared memory implementation of share mode locking

   21-Jul-1998: rsharpe@ns.aus.com (Richard Sharpe)
   Added -L (locks only) -S (shares only) flags and code

*/

/*
 * This program reports current SMB connections
 */

#define NO_SYSLOG

#include "includes.h"

#define	MOUNT_PATH	"/harddisk/volume_6_/data"
#define SMB_MAXPIDS		512
static pid_t		Ucrit_pid[SMB_MAXPIDS];  /* Ugly !!! */   /* added by OH */
static pstring 		Ucrit_username = "";               /* added by OH */
static int		Ucrit_MaxPid=0;                    /* added by OH */
static unsigned int	Ucrit_IsActive = 0;                /* added by OH */
static uid_t 		Ucrit_uid = 0;               /* added by OH */

char prefix[12]={0};

#define TMP_STATUS	"/tmp/smb.status"
#define TMP_LOCKED  "/tmp/smb.locked"


#if 0
/* added by OH */
static void Ucrit_addUsername(const char *username)
{
	pstrcpy(Ucrit_username, username);	
	if ( strlen(Ucrit_username) > 0 )
		Ucrit_IsActive = 1;
}
#endif

static unsigned int Ucrit_checkUid(uid_t uid)
{
	if ( !Ucrit_IsActive ) 
		return 1;
	
	if ( uid == Ucrit_uid ) 
		return 1;
	
	return 0;
}

static unsigned int Ucrit_checkUsername(const char *username)
{
	if ( !Ucrit_IsActive ) 
		return 1;
	
	if ( strcmp(Ucrit_username,username) == 0 ) 
		return 1;
	
	return 0;
}

static BOOL Ucrit_addPid( pid_t pid )
{
	if ( Ucrit_MaxPid >= SMB_MAXPIDS ) {
		d_printf("ERROR: More than %d pids for user %s!\n",
			SMB_MAXPIDS, Ucrit_username);

		return False;
	}

	Ucrit_pid[Ucrit_MaxPid++] = pid;
	
	return True;
}

static void print_share_mode(const struct share_mode_entry *e, const char *sharepath, const char *fname)
{
	char path[64]={0};		
	
	if(!fname||!fname[0])
		return;
#undef sprintf	
	sprintf(path,"%s/%s",MOUNT_PATH,prefix);
	if(!strncmp(fname,path,strlen(path))){
		printf("print_share_mode:pid '%d'\n",e->pid);
		Ucrit_addPid(procid_to_pid(&e->pid));
	}

	return;
}

static int traverse_fn1(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void *state)
{
	struct connections_data crec;
		
	if (dbuf.dsize != sizeof(crec))
		return 0;

	memcpy(&crec, dbuf.dptr, sizeof(crec));
	if (crec.cnum == -1)
		return 0;

	if (!process_exists(crec.pid)) {
		return 0;
	}
	printf("crec.name=%s,prefix=%s\n",crec.name,prefix);
	printf("crec.uid=%d\n",crec.uid);
	if(strstr(prefix, "FLASH_") || strstr(prefix, "HDD_")){
		if( !strncmp(crec.name,prefix,strlen(prefix)) ){
			printf("traverse_fn1: pid '%d'\n",crec.pid);
			Ucrit_addPid(procid_to_pid(&crec.pid));
		}
	}
	else{
		if( crec.uid==atoi(prefix) ){
			printf("traverse_fn1: pid '%d'\n",crec.pid);
			Ucrit_addPid(procid_to_pid(&crec.pid));
		}
	}
	  
	return 0;
}

static int traverse_sessionid(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void *state)
{
	struct sessionid sessionid;
	
	if (dbuf.dsize != sizeof(sessionid))
		return 0;

	memcpy(&sessionid, dbuf.dptr, sizeof(sessionid));

	if (!process_exists_by_pid(sessionid.pid) || !Ucrit_checkUid(sessionid.uid)) {
		return 0;
	}

	return 0;
}

 int main(int argc, char *argv[])
{
	int ret=0,num=0;
	TDB_CONTEXT *tdb;
	
	if(argc!=2){
		printf("Usage: %s [FLASH_X_Y|HDD_X_Y]\n",argv[0]);
		return(1);
	}
	nice(-10);
//	setup_logging(argv[0],True);
	memset(prefix,'\0',sizeof(prefix));
	strncpy(prefix,argv[1],sizeof(prefix)-1);
	dbf = x_stderr;
#if 0	
	if (getuid() != geteuid()) {
		d_printf("smbstatus should not be run setuid\n");
		return(1);
	}
#endif	
	if (!lp_load(dyn_CONFIGFILE,True,False,False)) {
		fprintf(stderr, "Can't load %s - run testparm to debug it\n", dyn_CONFIGFILE);
		return (-1);
	}
	tdb = tdb_open_log(lock_path("sessionid.tdb"), 0, TDB_DEFAULT, O_RDONLY, 0);
	if (!tdb) {
		d_printf("sessionid.tdb not initialised\n");
	}
	else {
		tdb_traverse(tdb, traverse_sessionid, NULL);
		tdb_close(tdb);
	}
	tdb = tdb_open_log(lock_path("connections.tdb"), 0, TDB_DEFAULT, O_RDONLY, 0);
	if (!tdb) {
		d_printf("%s not initialised\n", lock_path("connections.tdb"));
		d_printf("This is normal if an SMB client has never connected to your server.\n");
	}
	else  {
		tdb_traverse(tdb, traverse_fn1, NULL);
		tdb_close(tdb);
	}
	if (!locking_init(1)) {
		d_printf("Can't initialise locking module - exiting\n");
	}
	ret = share_mode_forall(print_share_mode);
	if (ret == 0) {
		d_printf("No locked files\n");
	}
	else if (ret == -1) {
		d_printf("locked file list truncated\n");
	}
	while(num<Ucrit_MaxPid){
		kill(Ucrit_pid[num],SIGTERM);
		sleep(1);
		if(process_exists_by_pid(Ucrit_pid[num]))
			kill(Ucrit_pid[num],SIGTERM);		
		num++;
	}
	d_printf("\n");		
	locking_end();

	return (0);
}
