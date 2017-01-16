#ifndef _MAIN_H_
#define _MAIN_H_

#include "parsedir.h"
#include <sys/types.h>

struct bftpd_childpid {
	pid_t pid;
	int sock;
};

extern int global_argc;
extern char **global_argv;
extern struct sockaddr_in name;
extern FILE *passwdfile, *groupfile, *devnull;
extern char *remotehostname;
extern struct sockaddr_in remotename;
extern char noatexit;
extern int control_timeout, data_timeout;
extern int alarm_type;
extern usershare *share;
extern sh_list *shinfo;
/* Command line options */
extern char *configpath;
extern char daemonmode;

void print_file(int number, char *filename);
#endif

