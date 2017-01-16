#include <config.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_DIRENT_H
#  include <dirent.h>
#else
#    define dirent direct
#    define NAMLEN(dirent) (dirent)->d_namlen
#  ifdef HAVE_SYS_NDIR_H
#    include <sys/ndir.h>
#  endif
#  ifdef HAVE_SYS_DIR_H
#    include <sys/dir.h>
#  endif
#  ifdef HAVE_NDIR_H
#    include <ndir.h>
#  endif
#endif

#include <unistd.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <stdarg.h>
#include <string.h>
#include <mystring.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
//#ifdef HAVE_TIME_H
#include <time.h>
//#endif
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <glob.h>

#include "options.h"
#include "main.h"
#include "login.h"
#include "logging.h"
#include "parsedir.h"
#include "libutil.h"

void bftpd_stat(char *name, FILE * client)
{
    struct stat statbuf;
	char temp[MAXCMD + 3], linktarget[MAXCMD + 5], perm[11], timestr[17],
		uid[USERLEN + 1], gid[USERLEN + 1];
    struct tm filetime;
    time_t t;
    
    if (lstat(name, (struct stat *) &statbuf) == -1) { // used for command_stat
        fprintf(client, "213-%s.\n", strerror(errno));
        return;
    }
#ifdef S_ISLNK
	if (S_ISLNK(statbuf.st_mode)) {
		strcpy(perm, "lrwxrwxrwx");
		temp[readlink(name, temp, sizeof(temp) - 1)] = '\0';
		sprintf(linktarget, " -> %s", temp);
	} else {
#endif
		strcpy(perm, "----------");
		if (S_ISDIR(statbuf.st_mode))
			perm[0] = 'd';
		if (statbuf.st_mode & S_IRUSR)
			perm[1] = 'r';
		if (statbuf.st_mode & S_IWUSR)
			perm[2] = 'w';
		if (statbuf.st_mode & S_IXUSR)
			perm[3] = 'x';
		if (statbuf.st_mode & S_IRGRP)
			perm[4] = 'r';
		if (statbuf.st_mode & S_IWGRP)
			perm[5] = 'w';
		if (statbuf.st_mode & S_IXGRP)
			perm[6] = 'x';
		if (statbuf.st_mode & S_IROTH)
			perm[7] = 'r';
		if (statbuf.st_mode & S_IWOTH)
			perm[8] = 'w';
		if (statbuf.st_mode & S_IXOTH)
			perm[9] = 'x';
		linktarget[0] = '\0';
#ifdef S_ISLNK
	}
#endif
    memcpy(&filetime, localtime(&(statbuf.st_mtime)), sizeof(struct tm));
    time(&t);
    if (filetime.tm_year == localtime(&t)->tm_year)
    	mystrncpy(timestr, ctime(&(statbuf.st_mtime)) + 4, 12);
    else
        strftime(timestr, sizeof(timestr), "%b %d  %G", &filetime);
    mygetpwuid(statbuf.st_uid, passwdfile, uid)[8] = 0;
    mygetpwuid(statbuf.st_gid, groupfile, gid)[8] = 0;
#ifdef __USE_UNICODE__
    from_utf8(name);
#endif 
    fprintf(client, "%s %3i %-8s %-8s %8llu %s %s%s\r\n", perm,
			(int) statbuf.st_nlink, uid, gid,
			(unsigned long long) statbuf.st_size,
			timestr, name, linktarget);
}

void dirlist(char *name, FILE * client, char verbose,int convert)
{
    DIR *directory;
    char cwd[MAX_FILE_LENGTH],modtime[16]={0};
    int i,ret=0;
    glob_t globbuf;
    char *dirname2=NULL;
    usershare *current;
    time_t dt;
    struct tm dc;
	
    if ((strstr(name, "/.")) && strchr(name, '*'))
		return; /* DoS protection */
    getcwd(cwd, sizeof(cwd) - 1);   
    if((!strcmp(cwd,"/")&&!strcmp(name,"*"))||strcmp(name,"/")==0||(!strcmp(cwd,"/")&&!strcmp(name,"."))||(!strcmp(cwd,"/")&&!strcmp(name,"./"))){  
		current=share; 
		time(&dt);
		localtime_r(&dt,&dc);
		if(strftime(modtime,16,"%b %d %H:%M",&dc)!=12)
			strcpy(modtime,"Jan 1 00:00");    
   	 	while(current){ 
#ifndef NO_GROUP   	 	    
            fprintf(client, "%s %3i %-8s %-8s %8lu %s %s\r\n","drwxrwxr-x",1, user, "everyone",(unsigned long)4096,modtime,current->share);
#else
            fprintf(client, "%s %3i %-8s %-8s %8lu %s %s\r\n","drwxrwxr-x",1, user, "root",(unsigned long)4096,modtime,current->share);
#endif            
            current=current->next;
         }
         return;
    }
#ifdef __USE_UNICODE__
    to_utf8(name);
#endif     

	if (convert && (!strcmp(cwd, "/") || (name[0] == '/' && name[1] != '\0'))){//Absolute Path
		dirname2 = convertDir(name, share, NULL);
		if(!dirname2)
			dirname2=name;
		trans_dir(dirname2);
	}
	else{
		char *pFullPath=NULL;
			
		dirname2 = name;
		if(strlen(dirname2)){
			pFullPath=malloc(strlen(cwd)+strlen(dirname2)+2);
			if(pFullPath){
				sprintf(pFullPath, "%s/%s", cwd, dirname2);
				ret=trans_dir_with_prefix(cwd, pFullPath);
				if(!ret){
					strcpy(dirname2, pFullPath+strlen(cwd)+1);
				}		
				free(pFullPath);	
				pFullPath=NULL;
			}	
		}		
	}
  	if(strlen(cwd) + strlen(dirname2) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long directory.");
		if (dirname2 != name)
			free (dirname2);			
		return;
	}		
	if(strlen(dirname2) && strcmp(dirname2, "*") && access(dirname2, F_OK)){
		control_printf(SL_FAILURE,"550 The directory doesn't exist.");
		if (dirname2 != name)
			free (dirname2);			
		return;
	}    
    if((directory = opendir(dirname2))) {
		closedir(directory);
        chdir(dirname2);
        glob("*", 0, NULL, &globbuf);
    }
    else{
    	glob(name, 0, NULL, &globbuf);
    }
    
    for (i = 0; i < globbuf.gl_pathc; i++){
    	if(IsHiddenFile(globbuf.gl_pathv[i]))
    		continue; 	
        if (verbose)
            bftpd_stat(globbuf.gl_pathv[i], client);
        else{	
#ifdef __USE_UNICODE__
    		from_utf8(globbuf.gl_pathv[i]);
#else
			to_utf8(globbuf.gl_pathv[i]);
#endif 
			fprintf(client, "%s\r\n", globbuf.gl_pathv[i]);
		}
    }
    chdir(cwd);
    globfree(&globbuf);
    if(dirname2!=name)
        free(dirname2);
}

