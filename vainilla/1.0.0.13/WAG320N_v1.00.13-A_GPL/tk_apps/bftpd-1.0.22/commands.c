#include <config.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#define __USE_GNU
#include <unistd.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_ASM_SOCKET_H
#include <asm/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_WAIT_H
# include <wait.h>
#else
# ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
# endif
#endif
#ifdef HAVE_SYS_SENDFILE_H
# include <sys/sendfile.h>
#endif


#include "mystring.h"
#include "login.h"
#include "logging.h"
#include "dirlist.h"
#include "options.h"
#include "main.h"
#include "targzip.h"
#include "parsedir.h"
#include "libcase.h"
#include "libutil.h"
#include "filename.h"

#ifdef HAVE_ZLIB_H
# include <zlib.h>
#else
# undef WANT_GZIP
#endif
int state = STATE_CONNECTED;
char user[USERLEN + 1];
struct sockaddr_in sa;
char pasv = 0;
int sock;
int pasvsock;
char philename[MAX_FILE_LENGTH+1]={0};
unsigned long long offset = 0;
short int xfertype = TYPE_BINARY;
int ratio_send = 1, ratio_recv = 1;
int bytes_sent = 0, bytes_recvd = 0;
int epsvall = 0;
int xfer_bufsize;
char pwd[MAX_FILE_LENGTH+1]="/";
char currentshare[32];
extern void to_utf8(char *file_name);
extern void from_utf8(char *file_name);
extern int utf8_support;
int client_noted=0;
int file_zilla=0;

void control_printf(char success, char *format, ...)
{
    char buffer[MAX_FILE_LENGTH+1]={0};
    va_list val;
    va_start(val, format);
    vsnprintf(buffer, sizeof(buffer), format, val);
    va_end(val);
    strcat(buffer, "\r\n");
    write(2, buffer, strlen(buffer));
    //fputs(buffer, stderr, "%s\r\n", buffer);
    //replace(buffer, "\r", "");
	//bftpd_statuslog(3, success, "%s", buffer);
}

void new_umask()
{
    int um;
    char *foo = config_getoption("UMASK");
    if (!foo[0])
        um = 022;
    else
        um = strtoul(foo, NULL, 8);
    umask(um);
}

void prepare_sock(int sock)
{
    int on = 1;
#ifdef TCP_NODELAY
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void *) &on, sizeof(on));
#endif
#ifdef TCP_NOPUSH
	setsockopt(sock, IPPROTO_TCP, TCP_NOPUSH, (void *) &on, sizeof(on));
#endif
#ifdef SO_REUSEADDR
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *) &on, sizeof(on));
#endif
#ifdef SO_REUSEPORT
	setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, (void *) &on, sizeof(on));
#endif
#ifdef SO_SNDBUF
	on = 65536;
	setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (void *) &on, sizeof(on));
#endif
}

int dataconn()
{
	struct sockaddr foo;
	struct sockaddr_in local;
	int namelen = sizeof(foo);
	int curuid = geteuid();

	memset(&foo, 0, sizeof(foo));
	memset(&local, 0, sizeof(local));

	if (pasv) {
		sock = accept(pasvsock, (struct sockaddr *) &foo, (int *) &namelen);
		if (sock == -1) {
			control_printf(SL_FAILURE, "425-Unable to accept data connection.\r\n425 %s.",
			strerror(errno));
			return 1;
		}
		close(pasvsock);
		pasvsock=-1;
		prepare_sock(sock);
	} 
	else {
		sock = socket(AF_INET, SOCK_STREAM, 0);
                prepare_sock(sock);
		local.sin_addr.s_addr = name.sin_addr.s_addr;
		local.sin_family = AF_INET;
        if (!strcasecmp(config_getoption("DATAPORT20"), "yes")) {
               seteuid(0);
               local.sin_port = htons(20);
        }
	if (bind(sock, (struct sockaddr *) &local, sizeof(local)) < 0) {
		control_printf(SL_FAILURE, "425-Unable to bind data socket.\r\n425 %s.",
					strerror(errno));
		return 1;
	}
        if (!strcasecmp(config_getoption("DATAPORT20"), "yes"))
                seteuid(curuid);
		sa.sin_family = AF_INET;
		if (connect(sock, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
			control_printf(SL_FAILURE, "425-Unable to establish data connection.\r\n"
                    "425 %s.", strerror(errno));
			return 1;
		}
	}
	control_printf(SL_SUCCESS, "150 %s data connection established.",
	               xfertype == TYPE_BINARY ? "BINARY" : "ASCII");
	return 0;
}

void init_userinfo()
{
    struct passwd *temp = getpwnam(user);
    if (temp) {
        userinfo.pw_name = strdup(temp->pw_name);
        userinfo.pw_passwd = strdup(temp->pw_passwd);
        userinfo.pw_uid = temp->pw_uid;
        userinfo.pw_gid = temp->pw_gid;
        userinfo.pw_gecos = strdup(temp->pw_gecos);
        userinfo.pw_dir = strdup(temp->pw_dir);
        userinfo.pw_shell = strdup(temp->pw_shell);
        userinfo_set = 1;
    }
}


int my_anonymous_flag = 0;
void command_user(char *username)
{
	char *alias;

	if (state) {
		control_printf(SL_FAILURE, "503 Username already given.");
		return;
	}
	mystrncpy(user, username, sizeof(user) - 1);
	if(!strlen(username)){
		control_printf(SL_FAILURE, "421 Unknown user.");
		exit(0);		
	}
	if(username[0]=='-'){
		control_printf(SL_FAILURE, "421 Unknown user.");
		exit(0);		
	}
	//but anonymous user will not follow this setting.
        userinfo_set = 1; /* Dirty! */
	alias = (char *) config_getoption("ALIAS");
        userinfo_set = 0;
	if (alias[0] != '\0')
		mystrncpy(user, alias, sizeof(user) - 1);

        init_userinfo();
#ifdef DEBUG
	bftpd_log("Trying to log in as %s.\n", user);
#endif
        expand_groups();
	if (!strcasecmp(config_getoption("ANONYMOUS_USER"), "yes")) {
		my_anonymous_flag = 1;
		control_printf(SL_SUCCESS, "331 Password please.");
		bftpd_login("");
	}
	else {
		state = STATE_USER;
		control_printf(SL_SUCCESS, "331 Password please.");
	}
}

void command_pass(char *password)
{
	if (state > STATE_USER) {
		if (my_anonymous_flag==1)
			return;
		control_printf(SL_FAILURE, "503 Already logged in.");
		return;
	}
	if (bftpd_login(password)) {
#ifdef DEBUG		
		bftpd_log("Login as user '%s' failed.\n", user);
#endif		
		control_printf(SL_FAILURE, "421 Login incorrect.");
		exit(0);
	}
}

void command_pwd(char *params)
{
	char xpwd[MAX_FILE_LENGTH+1]={0};
	
	strcpy(xpwd,pwd);
#ifdef __USE_UNICODE__
	from_utf8(xpwd);
#endif
 	control_printf(SL_SUCCESS, "257 \"%s\" is the current working directory.",xpwd);

}

void command_type(char *params)
{
    if ((*params == 'A') || (*params == 'a')) {
        control_printf(SL_SUCCESS, "200 Transfer type changed to ASCII");
        xfertype = TYPE_ASCII;
    } else if ((*params == 'I') || (*params == 'i')) {
      	control_printf(SL_SUCCESS, "200 Transfer type changed to BINARY");
        xfertype = TYPE_BINARY;
    } else
        control_printf(SL_FAILURE, "500 Type '%c' not supported.", *params);
}

void command_port(char *params) {
  unsigned long a0, a1, a2, a3, p0, p1, addr;
  if (epsvall) {
      control_printf(SL_FAILURE, "500 EPSV ALL has been called.");
      return;
  }
  sscanf(params, "%lu,%lu,%lu,%lu,%lu,%lu", &a0, &a1, &a2, &a3, &p0, &p1);
  addr = htonl((a0 << 24) + (a1 << 16) + (a2 << 8) + a3);
  if((addr != remotename.sin_addr.s_addr) &&( strncasecmp(config_getoption("ALLOW_FXP"), "yes", 3))) {
      control_printf(SL_FAILURE, "500 The given address is not yours.");
      return;
  }
  sa.sin_addr.s_addr = addr;
  sa.sin_port = htons((p0 << 8) + p1);
  if (pasv) {
    close(sock);
    sock=-1;
    pasv = 0;
  }
  control_printf(SL_SUCCESS, "200 PORT %lu.%lu.%lu.%lu:%lu OK",
           a0, a1, a2, a3, (p0 << 8) + p1);
}

void command_eprt(char *params) {
    char delim;
    int af;
    char addr[51];
    char foo[20];
    int port;
    if (epsvall) {
        control_printf(SL_FAILURE, "500 EPSV ALL has been called.");
        return;
    }
    if (strlen(params) < 5) {
        control_printf(SL_FAILURE, "500 Syntax error.");
        return;
    }
    delim = params[0];
    sprintf(foo, "%c%%i%c%%50[^%c]%c%%i%c", delim, delim, delim, delim, delim);
    if (sscanf(params, foo, &af, addr, &port) < 3) {
        control_printf(SL_FAILURE, "500 Syntax error.");
        return;
    }
    if (af != 1) {
        control_printf(SL_FAILURE, "522 Protocol unsupported, use (1)");
        return;
    }
    sa.sin_addr.s_addr = inet_addr(addr);
    if ((sa.sin_addr.s_addr != remotename.sin_addr.s_addr) && (strncasecmp(config_getoption("ALLOW_FXP"), "yes", 3))) {
        control_printf(SL_FAILURE, "500 The given address is not yours.");
        return;
    }
    sa.sin_port = htons(port);
    if (pasv) {
        close(sock);
        sock=-1;
        pasv = 0;
    }
    control_printf(SL_FAILURE, "200 EPRT %s:%i OK", addr, port);
}

void command_pasv(char *foo)
{
	int namelen, a1, a2, a3, a4;
	struct sockaddr_in localsock;
	
        if (epsvall) {
          control_printf(SL_FAILURE, "500 EPSV ALL has been called.");
          return;
        }
	pasvsock = socket(AF_INET, SOCK_STREAM, 0);
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_family = AF_INET;

        if (!config_getoption("PASSIVE_PORTS") || !strlen(config_getoption("PASSIVE_PORTS"))) {
        /* bind to any port */
           sa.sin_port = 0;
           if (bind(pasvsock, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
              control_printf(SL_FAILURE, "425-Unable to bind data socket.\r\n425 %s", strerror(errno));
              return;
           }
	} 
	else {
            int i = 0, success = 0, port;
            for (;;) {
              port = int_from_list(config_getoption("PASSIVE_PORTS"), i++);
              if (port < 0)
                break;
               sa.sin_port = htons(port);
               if (bind(pasvsock, (struct sockaddr *) &sa, sizeof(sa)) == 0) {
                 success = 1;
#ifdef DEBUG
                 bftpd_log("Passive mode: Successfully bound port %d\n", port);
#endif
                break;
              }
           }
           if (!success) {
              control_printf(SL_FAILURE, "425 Unable to bind data socket.");
              return;
        }
        prepare_sock(pasvsock);
    }       
	if (listen(pasvsock, 1)) {
		control_printf(SL_FAILURE, "425-Unable to make socket listen.\r\n425 %s",
				 strerror(errno));
		return;
	}
	namelen = sizeof(localsock);
	getsockname(pasvsock, (struct sockaddr *) &localsock, (int *) &namelen);
	sscanf((char *) inet_ntoa(name.sin_addr), "%i.%i.%i.%i",
		   &a1, &a2, &a3, &a4);
	control_printf(SL_SUCCESS, "227 Entering Passive Mode (%i,%i,%i,%i,%i,%i)", a1, a2, a3, a4,
			 ntohs(localsock.sin_port) >> 8, ntohs(localsock.sin_port) & 0xFF);
	pasv = 1;
}

void command_epsv(char *params)
{
    struct sockaddr_in localsock;
    int namelen;
    int af;
    if (params[0]) {
        if (!strncasecmp(params, "ALL", 3))
            epsvall = 1;
        else {
            if (sscanf(params, "%i", &af) < 1) {
                control_printf(SL_FAILURE, "500 Syntax error.");
                return;
            } else {
                if (af != 1) {
                    control_printf(SL_FAILURE, "522 Protocol unsupported, use (1)");
                    return;
                }
            }
        }
    }
    pasvsock = socket(AF_INET, SOCK_STREAM, 0);
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = 0;
    sa.sin_family = AF_INET;
    if (bind(pasvsock, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
	 control_printf(SL_FAILURE, "500 Unable to bind data socket.\r\n425 %s",
				 strerror(errno));
	 return;
    }
    if (listen(pasvsock, 1)) {
	 control_printf(SL_FAILURE, "500 Unable to make socket listen.\r\n425 %s",
				 strerror(errno));
	 return;
    }
    namelen = sizeof(localsock);
    getsockname(pasvsock, (struct sockaddr *) &localsock, (int *) &namelen);
    control_printf(SL_SUCCESS, "229 Entering extended passive mode (|||%i|)",
             ntohs(localsock.sin_port));
    pasv = 1;
}

char test_abort(char selectbefore, int file, int sock)
{
    char str[MAX_FILE_LENGTH+1]={0};
    fd_set rfds;
    struct timeval tv;
    
    if (selectbefore) {
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        FD_ZERO(&rfds);
        FD_SET(fileno(stdin), &rfds);
        if (!select(fileno(stdin) + 1, &rfds, NULL, NULL, &tv))
            return 0;
    }
    fgets(str, sizeof(str), stdin);
    if (strstr(str, "ABOR")) {
        control_printf(SL_SUCCESS, "426 Transfer aborted.");
    	close(file);
	close(sock);
	sock=-1;
   	control_printf(SL_SUCCESS, "226 Aborted.");
#ifdef DEBUG   		
	bftpd_log("Client aborted file transmission.\n");
#endif		
        alarm(control_timeout);
        return 1;
    }
    return 0;
}

void command_allo(char *foo)
{
    command_noop(foo);
}

extern int store_deny;

void do_stor(char *filename, int flags)
{
	char *buffer=NULL;
	int fd, i, max,ret=0,readonly=0;
	fd_set rfds;
	struct timeval tv;
	char *p, *pp;
	char *filename1=NULL;
	char cwd[MAX_FILE_LENGTH+1]={0};
    int stdin_fileno=0;

#ifdef __USE_UNICODE__
	to_utf8(filename);
#endif         
	ret=checkFolder(filename,share,&readonly,NULL,1);
	switch(ret){
		case 1:
			if(file_zilla)
				store_deny=1;		
 			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			if(readonly){//Read/Write
				break;
			}
		case 2:
			if(file_zilla)
				store_deny=1;
			if(!readonly)
				control_printf(SL_FAILURE,"532 Permission denied.");
			else
				control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}   
	getcwd(cwd,sizeof(cwd)-1);
	if(!strcmp(cwd,"/")||filename[0]=='/') {                                      
		filename1=convertDir(filename,share,currentshare);
		if(!filename1)
			filename1=filename;
		trans_dir(filename1);
	}
	else{
		char *pFullPath=NULL;
				
		filename1=filename;
		pFullPath=malloc(strlen(cwd)+strlen(filename1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, filename1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(filename1, pFullPath+strlen(cwd)+1);
			}		
			free(pFullPath);
			pFullPath=NULL;							
		}			
	}	
	if(strlen(cwd) + strlen(filename1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"553 Too long file name.");
		if (filename1 != filename)
			free (filename1);			
			return;
	}	

	if(filename1[0]!='/'){
		strcat(cwd,"/");
		strcat(cwd,filename1);
	}
	else
		strcpy(cwd,filename1);
		
	fd = open(cwd, flags, 00664);
	if(fd == -1) {
#ifdef DEBUG		
		bftpd_log("'%s' while trying to store file '%s'.\n",
				  strerror(errno), cwd);
#endif				 
		if(errno==EDQUOT) 
			control_printf(SL_FAILURE, "552 %s.", strerror(errno));
		else
			control_printf(SL_FAILURE, "550 %s.", strerror(errno));
		goto ret;
	}
#ifdef DEBUG	
	bftpd_log("Client is storing file '%s'.\n", filename);
#endif	
	if(dataconn()){
		close(fd);
		goto ret;
	}
	alarm(0);
	buffer = malloc(xfer_bufsize);
	if(!buffer){
	     close(fd);
	     goto ret;
	}
	lseek(fd, offset, SEEK_SET);
	offset = 0;
	/* Do not use the whole buffer, because a null byte has to be
	* written after the string in ASCII mode. */
    stdin_fileno = fileno(stdin);
	max = (sock > stdin_fileno ? sock : stdin_fileno) + 1;
	for (;;) {
		FD_ZERO(&rfds);
		FD_SET(sock, &rfds);
		FD_SET(stdin_fileno, &rfds);
        
		tv.tv_sec = data_timeout;
		tv.tv_usec = 0;
		if (!select(max, &rfds, NULL, NULL, &tv)) {
			close(sock);
			sock=-1;
			close(fd);
			control_printf(SL_FAILURE, "426 Kicked due to data transmission timeout.");
#ifdef DEBUG            
			bftpd_log("Kicked due to data transmission timeout.\n");
#endif            
			if (buffer){
				free(buffer);    
				buffer=NULL;
			}
			return;
		}
		if (FD_ISSET(stdin_fileno, &rfds)) {
			test_abort(0, fd, sock);
			close(sock);
			close(fd);
			sock=-1;
			goto ret;
		}
		if (!((i = recv(sock, buffer, xfer_bufsize - 1, 0))))
             break;
		bytes_recvd += i;
		if (xfertype == TYPE_ASCII) {
			buffer[i] = '\0';
            p = pp = buffer;
    	    while (*p) {
				if ((unsigned char) *p == 13)
					p++;
				else
					*pp++ = *p++;
			}
			*pp++ = 0;
			i = strlen(buffer);
		}
		ret=write(fd, buffer, i);
		if(ret==-1){ 
			close(sock);
			sock=-1;
			close(fd);
			if(errno==EDQUOT) 
				control_printf(SL_FAILURE, "552 %s.", strerror(errno));
			else
				control_printf(SL_FAILURE, "550 %s.", strerror(errno));
			goto ret;
		}   
	}
	close(fd);
	close(sock);
	sock=-1;
	alarm(control_timeout);
	offset = 0;
	control_printf(SL_SUCCESS, "226 File transmission successful.");
#ifdef DEBUG	
	bftpd_log("File transmission successful.\n");
#endif	
ret:
	if (buffer){
	    free(buffer);
	    buffer=NULL;
	}
	if(filename1!=filename)
		free(filename1);
	return;
}

void command_stor(char *filename)
{
    do_stor(filename, O_CREAT | O_WRONLY | O_TRUNC);
}

void command_appe(char *filename)
{
    do_stor(filename, O_CREAT | O_WRONLY | O_APPEND);
}

void command_retr(char *filename)
{
	char *buffer;
#ifdef WANT_GZIP
    gzFile gzfile;
#endif
	int phile;
	int i, whattodo = DO_NORMAL;
	struct stat statbuf;
#ifdef HAVE_SYS_SENDFILE_H
        off_t sendfile_offset;
#endif
#if (defined(WANT_TAR) && defined(WANT_GZIP))
        int filedes[2];
#endif
#if (defined(WANT_TAR) || defined(WANT_GZIP))
        char *foo;
#endif
#ifdef WANT_TAR
        char *argv[4];
#endif
	int ret=0;
	char *filename1=NULL,cwd[MAX_FILE_LENGTH+1]={0},*pFullPath=NULL;
         
#ifdef __USE_UNICODE__
	to_utf8(filename);
#endif   
	ret=checkFolder(filename,share,NULL,NULL,1);
	switch(ret){
		case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			break;
		case 2:
			control_printf(SL_FAILURE,"550 Permission denied.");
		return;
	}	
	getcwd(cwd,sizeof(cwd)-1);
		if(!strcmp(cwd,"/")||filename[0]=='/'){      
			filename1=convertDir(filename,share,NULL);
			if(!filename1)
				filename1=filename;	
			trans_dir(filename1);
		}
		else{
			filename1=filename;	
			pFullPath=malloc(strlen(cwd)+strlen(filename1)+2);
			if(pFullPath){
				sprintf(pFullPath, "%s/%s", cwd, filename1);
				ret=trans_dir_with_prefix(cwd, pFullPath);
				if(!ret){
					strcpy(filename1, pFullPath+strlen(cwd)+1);
				}			
				free(pFullPath);	
				pFullPath=NULL;		
			}		
		}
		if(strlen(cwd) + strlen(filename1) > MAX_FILE_LENGTH){
			control_printf(SL_FAILURE, "553 Too long filename.");
			if (filename1 != filename)
				free (filename1);			
			return;
		}
		if(access(filename1, F_OK)){
			control_printf(SL_FAILURE, "553 The file doesn't exist.");
			if (filename1 != filename)
				free (filename1);				
			return;
		}
		  
	if (((phile = open(filename1, O_RDONLY))) == -1) {
#if (defined(WANT_TAR) && defined(WANT_GZIP))
		if ((foo = strstr(filename1, ".tar.gz")))
			if (strlen(foo) == 7) {
				whattodo = DO_TARGZ;
				*foo = '\0';
			}
#endif
#ifdef WANT_TAR
		if ((foo = strstr(filename1, ".tar")))
			if (strlen(foo) == 4) {
				whattodo = DO_TARONLY;
				*foo = '\0';
			}
#endif
#ifdef WANT_GZIP
		if ((foo = strstr(filename1, ".gz")))
			if (strlen(foo) == 3) {
				whattodo = DO_GZONLY;
				*foo = '\0';
			}
#endif
		if (whattodo == DO_NORMAL) {
#ifdef DEBUG			
			bftpd_log("'%s' while trying to receive file '%s'.\n",
					  strerror(errno), filename);
#endif					  
			control_printf(SL_FAILURE, "553 %s.", strerror(errno));
			goto ret;

		}
	} /* No else, the file remains open so that it needn't be opened again */
	if(stat(filename1, (struct stat *) &statbuf)==-1){
	   control_printf(SL_FAILURE, "553 %s.", strerror(errno));
	   goto ret;
	}  
	if(!S_ISREG(statbuf.st_mode)){
	   control_printf(SL_FAILURE, "550 %s is not a regular file.", filename); 
	   goto ret;
	}   
	if ((((statbuf.st_size - offset) * ratio_send) / ratio_recv > bytes_recvd
		 - bytes_sent) && (strcmp((char *) config_getoption("RATIO"), "none"))) {
#ifdef DEBUG		 	
		bftpd_log("Error: 'File too big (ratio)' while trying to receive file "
				  "'%s'.\n", filename);
#endif				  
		control_printf(SL_FAILURE, "553 File too big. Send at least %i bytes first.",
				(int) (((statbuf.st_size - offset) * ratio_send) / ratio_recv)
				- bytes_recvd);
		goto ret;
	}
#ifdef DEBUG	
	bftpd_log("Client is receiving file '%s'.\n", filename);
#endif	
	switch (whattodo) {
#if (defined(WANT_TAR) && defined(WANT_GZIP))
        case DO_TARGZ:
            close(phile);
            if (dataconn()) 	     
              goto ret;   
            alarm(0);
            pipe(filedes);
            if (fork()) {
                buffer = malloc(xfer_bufsize);
                close(filedes[1]);
                gzfile = gzdopen(sock, "wb");
                while ((i = read(filedes[0], buffer, xfer_bufsize))) {
                    gzwrite(gzfile, buffer, i);
                    test_abort(1, phile, sock);
                }
                free(buffer);
                gzclose(gzfile);
                wait(NULL); /* Kill the zombie */
            } else {
                stderr = devnull;
                close(filedes[0]);
                close(fileno(stdout));
                dup2(filedes[1], fileno(stdout));
                setvbuf(stdout, NULL, _IONBF, 0);
                argv[0] = "tar";
                argv[1] = "cf";
                argv[2] = "-";
                argv[3] = filename;
                ret=pax_main(4, argv);
                if(filename1!=filename)
		   free(filename1); 
                exit(ret);
            }
            break;
#endif
#ifdef WANT_TAR
		case DO_TARONLY:
            if (dataconn())
               goto ret;
            alarm(0);
            if (fork())
                wait(NULL);
            else {
                stderr = devnull;
                dup2(sock, fileno(stdout));
                argv[0] = "tar";
                argv[1] = "cf";
                argv[2] = "-";
                argv[3] = filename;
                ret=pax_main(4, argv);
                if(filename1!=filename)
		  free(filename1); 
                exit(ret);
            }
            break;
#endif
#ifdef WANT_GZIP
		case DO_GZONLY:
            if ((phile = open(filename1, O_RDONLY)) < 0) {
				control_printf(SL_FAILURE, "553 %s.", strerror(errno));
				return;
	    }
	    if (dataconn())
	       goto ret;
            alarm(0);
            buffer = malloc(xfer_bufsize);
            /* Use "wb9" for maximum compression, uses more CPU time... */
            gzfile = gzdopen(sock, "wb");
            while ((i = read(phile, buffer, xfer_bufsize))) {
                gzwrite(gzfile, buffer, i);
                test_abort(1, phile, sock);
            }
            free(buffer);
            close(phile);
            gzclose(gzfile);
            break;
#endif
	    case DO_NORMAL:
		if (dataconn())
		   goto ret;
            alarm(0);
#ifdef HAVE_SYS_SENDFILE_H
            sendfile_offset = offset;
            if (xfertype != TYPE_ASCII) {
                alarm_type = phile;
                while (sendfile(sock, phile, &sendfile_offset, xfer_bufsize)) {
                    alarm(data_timeout);
                    if (test_abort(1, phile, sock))
                       goto ret;
                }
                alarm(control_timeout);
                alarm_type = 0;
            } 
            else {
#endif
		 lseek(phile, offset, SEEK_SET);
		 offset = 0;
		 buffer = malloc(xfer_bufsize * 2 + 1);
		 while ((i = read(phile, buffer, xfer_bufsize))) {
                    if (test_abort(1, phile, sock))
                        goto ret;

#ifndef HAVE_SYS_SENDFILE_H
                   if (xfertype == TYPE_ASCII) {
#endif
                     buffer[i] = '\0';
                     i += replace(buffer, "\n", "\r\n");
#ifndef HAVE_SYS_SENDFILE_H
                   }
#endif
		   send(sock, buffer, i, 0);
		   bytes_sent += i;
	    }
            free(buffer);
#ifdef HAVE_SYS_SENDFILE_H
         }
#endif
	  close(phile);
	}
	close(sock);
	sock=-1;
	offset = 0;
	alarm(control_timeout);
   	control_printf(SL_SUCCESS, "226 File transmission successful.");
#ifdef DEBUG   	
	bftpd_log("File transmission successful.\n");
#endif	
ret:
	if(filename1!=filename)
	   free(filename1);
	return;
}

void do_dirlist(char *dirname, char verbose)
{
	FILE *datastream;
	int ret,convert=1;
 	char all_files[12]="*";
#ifdef __USE_UNICODE__
	to_utf8(dirname);
#endif    
	if (dirname[0] != '\0') {
		/* skip arguments */
		if (dirname[0] == '-') {
			while ((dirname[0] != ' ') && (dirname[0] != '\0'))
				dirname++;
			if (dirname[0] != '\0')
				dirname++;
		}
	}

	if(dirname&&dirname[0]!='\0'&&dirname[0]!='*'){
		ret=checkFolder(dirname,share,NULL,&convert,0);
		switch(ret){
			case 1:
				control_printf(SL_FAILURE,"550 No such file or directory.");
				return;	
			case 0:
				break;
			case 2:
				control_printf(SL_FAILURE,"550 Permission denied.");
				return;
		} 
	}
	if(dataconn())
	    return;
	alarm(0);
	datastream = fdopen(sock, "w");
	if (dirname[0] == '\0')
		dirlist(all_files, datastream, verbose,convert);
	else
		dirlist(dirname, datastream, verbose,convert);
	fclose(datastream);
	alarm(control_timeout);
	control_printf(SL_SUCCESS, "226 Directory list has been submitted.");
}

void command_list(char *dirname)
{
	do_dirlist(dirname, 1);
}

void command_nlst(char *dirname)
{
	do_dirlist(dirname, 0);
}

void command_syst(char *params)
{
	control_printf(SL_SUCCESS, "215 UNIX Type: L8");
}

void command_mdtm(char *filename)
{
	struct stat statbuf;
	struct tm *filetime;
	char *filename1=NULL,cwd[MAX_FILE_LENGTH+1]={0};
	int ret;

#ifdef __USE_UNICODE__
	to_utf8(filename);
#endif  	
	ret=checkFolder(filename,share,NULL,NULL,1);
	switch(ret){
        case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			break;
		case 2:
			control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}
	getcwd(cwd,sizeof(cwd)-1);
	if(!strcmp(cwd,"/")||filename[0]=='/') {       
		filename1=convertDir(filename,share,NULL);
		if(!filename1)
			filename1=filename;
		trans_dir(filename1);			 	
	}			 
	else{
		char *pFullPath=NULL;
			
		filename1=filename;
		pFullPath=malloc(strlen(cwd)+strlen(filename1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, filename1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(filename1, pFullPath+strlen(cwd)+1);
			}
			free(pFullPath);	
			pFullPath=NULL;				
		}
	}		  
	if(strlen(cwd) + strlen(filename1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long file name.");
		if (filename1 != filename)
			free (filename1);		
		return;
	}			
	if(access(filename1, F_OK)){
		control_printf(SL_FAILURE,"550 The file doesn't exist.");
		if (filename1 != filename)
			free (filename1);			
		return;
	} 
	if (!stat(filename1, (struct stat *) &statbuf)) {
		filetime = gmtime((time_t *) & statbuf.st_mtime);
		control_printf(SL_SUCCESS, "213 %04i%02i%02i%02i%02i%02i",
				filetime->tm_year + 1900, filetime->tm_mon + 1,
				filetime->tm_mday, filetime->tm_hour, filetime->tm_min,
				filetime->tm_sec);
	}
	 else {
		control_printf(SL_FAILURE, "550 Error while determining the modification time: %s", strerror(errno));
	}

	if(filename1!=filename)	
		free(filename1);
}

void command_cwd(char *dir)
{
	char *dir1=NULL;
	int len,ret,convert=1;
	char sharename[48],cwd[MAX_FILE_LENGTH+1]={0};
	char *pFullPath=NULL;
	
	if (dir[0] == '\0')
		strcpy(dir, "/");
	de_dotdot(dir);
	if(!strcmp(dir,"..")){
	    sprintf(sharename,"/%s/",currentshare);
	    if(strcmp(pwd,sharename)<=0||!strcmp(pwd,"/")){
	       ret=chdir("/");
	       if(ret)
	          control_printf(SL_FAILURE, "451 %s.", strerror(errno));
	       else{ 
	          strcpy(pwd,"/"); 
  	          control_printf(SL_SUCCESS, "250 OK");
	       }
	    }   
	    else{
	       ret=chdir("..");	
	       if(ret)
	          control_printf(SL_FAILURE, "451 %s.", strerror(errno));
	       else {
	       	  getUpDir(pwd);
  	          control_printf(SL_SUCCESS, "250 OK");
  	       }   
  	    }   
  	    return;
  	} 
  	if(!strcmp(dir,"/")){
  	   if(chdir("/"))
			control_printf(SL_FAILURE, "451 %s.", strerror(errno));
		else{
               strcpy(pwd,"/");
               control_printf(SL_SUCCESS, "250 OK");
		}
             		
		return;	
	}
	if(!strcmp(dir,".")){
		control_printf(SL_SUCCESS,"250 OK");
		return;
	}				     	
#ifdef __USE_UNICODE__
	to_utf8(dir);
#endif    			  
	ret=checkFolder(dir,share,NULL,&convert,0);
	switch(ret){
		case 1:
			if(file_zilla)
				store_deny=1;			
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			break;
		case 2:
			if(file_zilla)
				store_deny=1;			
			control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}	   
	getcwd(cwd,sizeof(cwd)-1);
	if(convert && (!strcmp(cwd,"/")||dir[0]=='/')) {
		dir1=convertDir(dir,share,currentshare);
		if(!dir1)
			dir1=dir;
		trans_dir(dir1);
	}
	else{
		dir1=dir;
		pFullPath=malloc(strlen(cwd)+strlen(dir1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, dir1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(dir1, pFullPath+strlen(cwd)+1);
			}
			free(pFullPath);
			pFullPath=NULL;
		}			
	}	
	if(strlen(cwd) + strlen(dir1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE, "451 Too long directory.");
		if(dir1!=dir)
		     free(dir1);		
		return;
	}		
	if(access(dir1, F_OK)){
		if(file_zilla)
			store_deny=1;			
		control_printf(SL_FAILURE, "451 The directory doesn't exists.");
		if(dir1!=dir)
		     free(dir1);		
		return;
	}
			
	if (chdir(dir1)) {
#ifdef DEBUG		
		bftpd_log("'%s' while changing directory to '%s'.\n",
				  strerror(errno), dir1);
#endif				  
		control_printf(SL_FAILURE, "451 %s.", strerror(errno));
	} 
	else {
		getcwd(cwd, sizeof(cwd) - 1); 
	    if(strcmp(cwd,"/usb_1")<0 && strcmp(cwd,"/usb_2")<0){
			chdir("/");
			strcpy(pwd,"/");
			control_printf(SL_SUCCESS, "250 OK");
			goto ret;
		}
#ifdef DEBUG	   
		bftpd_log("pwd is '%s'.\n", cwd);     
		bftpd_log("Changed directory to '%s'.\n", dir);
#endif		
		if(dir[0]=='/')
		    strcpy(pwd,dir);
		else{
		    if(!strcmp(pwd,"/")){
		    	 if(strcmp(dir,".."))
		    	    strcat(pwd,dir);
		    }     
		    else{
				strcat(pwd,"/");
				strcat(pwd,dir);
				len=strlen(pwd);
				if(pwd[len-1]=='/')
					pwd[len-1]=0;
			}      		          
		}
		control_printf(SL_SUCCESS, "250 OK");
		new_umask();
	}
ret:	
	if(dir1!=dir)
	     free(dir1);
	return;
}

void command_cdup(char *params)
{
	char share[48];
	
#ifdef DEBUG	
	bftpd_log("Changed directory to '..'.\n");
#endif	
	sprintf(share,"/%s/",currentshare);
	if(strcmp(pwd,share)<=0){
	   chdir("/");
	   strcpy(pwd,"/");
	}     
	else{
	   chdir("..");
	   getUpDir(pwd);
	}   
	control_printf(SL_SUCCESS, "250 OK");
	new_umask();
}

void command_dele(char *filename)
{
	char *filename1=NULL,cwd[MAX_FILE_LENGTH+1]={0};
	int ret,readonly=0;

#ifdef __USE_UNICODE__
	to_utf8(filename);
#endif  
	ret=checkFolder(filename,share,&readonly,NULL,1);
	switch(ret){
		case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			if(readonly) 
				break;
		case 2:
			if(!readonly)
				control_printf(SL_FAILURE,"532 Permission denied.");		
			else
				control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}
	getcwd(cwd,sizeof(cwd)-1);
	if(!strcmp(cwd,"/")||filename[0]=='/') {       
		filename1=convertDir(filename,share,NULL);
	       if(!filename1)
			filename1=filename;
		trans_dir(filename1);
	}
	else{
		char *pFullPath=NULL;
			
		filename1=filename;
		pFullPath=malloc(strlen(cwd)+strlen(filename1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, filename1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(filename1, pFullPath+strlen(cwd)+1);
			}
			free(pFullPath);	
			pFullPath=NULL;				
		}			
	}
	if(strlen(cwd) + strlen(filename1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long file name.");
		if (filename1 != filename)
			free (filename1);			
		return;
	}		
	if(access(filename1, F_OK)){
		control_printf(SL_FAILURE,"550 The file doesn't exist.");
		if (filename1 != filename)
			free (filename1);				
		return;
	}
	if (unlink(filename1)) {
#ifdef DEBUG		
		bftpd_log("Error: '%s' while trying to delete file '%s'.\n",
				  strerror(errno), filename);
#endif				  
		control_printf(SL_FAILURE, "451 %s.", strerror(errno));
	} else {
#ifdef DEBUG		
		bftpd_log("Deleted file '%s'.\n", filename);
#endif		
		control_printf(SL_SUCCESS, "200 OK");
	}

	if(filename1!=filename)
	     free(filename1);	
	return;	
}

void command_mkd(char *dirname)
{
	char *dir1=NULL;
	int ret,readonly=0;
	char cwd[MAX_FILE_LENGTH+1]={0}, *pFullPath=NULL;

#ifdef __USE_UNICODE__
	to_utf8(dirname);
#endif  
	ret=checkFolder(dirname,share,&readonly,NULL,1);
	switch(ret){
		case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			if(readonly)
				break;
		case 2:
			if(!readonly)
				control_printf(SL_FAILURE,"532 Permission denied.");		
			else
				control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}
	getcwd(cwd, sizeof(cwd) - 1);
	if(!strcmp(cwd,"/")||dirname[0]=='/') {		
		dir1=convertDir(dirname,share,NULL);
		if(!dir1)
			dir1=dirname;
		trans_dir(dir1);
	}
	else{
		dir1=dirname;
		pFullPath=malloc(strlen(cwd)+strlen(dir1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, dir1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(dir1, pFullPath+strlen(cwd)+1);
			}
			free(pFullPath);	
			pFullPath=NULL;				
		}
	}
	if(strlen(cwd) + strlen(dir1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long directory.");
		if (dir1 != dirname)
			free (dir1);			
		return;
	}
	if(!access(dir1, F_OK)){
		control_printf(SL_FAILURE,"550 The folder already exists.");
		if (dir1 != dirname)
			free (dir1);			
		return;
	}
#ifdef DEBUG	
	bftpd_log("dir1: %s\n", dir1);
#endif	
	/* prepend path to filename */
	if (dir1[0] != '/'){
		strcpy(cwd, pwd);
		strcat(cwd, "/");
		strcat(cwd, dir1);
	}
	else{
		strcpy(cwd, dir1);
	}
	if (mkdir(dir1,0777)) {
#ifdef DEBUG	
		bftpd_log("Error '%s' while trying to create directory '%s'.\n",
				  strerror(errno), dir1);
#endif				  
		if(errno==EDQUOT) 
			control_printf(SL_FAILURE, "552 %s.", strerror(errno));
		else
			control_printf(SL_FAILURE, "550 %s.", strerror(errno));
	} else {
#ifdef DEBUG		
		bftpd_log("Created directory '%s'.\n", dirname);
#endif		
		control_printf(SL_SUCCESS, "257 \"%s\" has been created.", dirname);
		chmod(dir1, 0777);
	}

	if(dir1!=dirname)
	     free(dir1);
	return;
}

void command_rmd(char *dirname)
{
	char *dir1=NULL,cwd[MAX_FILE_LENGTH+1]={0};
	int ret,readonly=0;

#ifdef __USE_UNICODE__
	to_utf8(dirname);
#endif  

#ifdef DEBUG            
	bftpd_log("dirname: %s.\n", dirname);
#endif 	
	ret=checkFolder(dirname,share,&readonly,NULL,1);
	switch(ret){
		case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			if(readonly)
				break;
		case 2:
			if(!readonly)
				control_printf(SL_FAILURE,"532 Permission denied.");		
			else
				control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}	
	getcwd(cwd, sizeof(cwd) - 1);
#ifdef DEBUG            
	bftpd_log("cwd: %s.\n", cwd);
#endif 		
	if(!strcmp(cwd,"/")||dirname[0]=='/') {		
		dir1=convertDir(dirname,share,NULL);
		if(!dir1)
			dir1=dirname;
		trans_dir(dir1);
	}
	else{
		char *pFullPath=NULL;
		
		dir1=dirname;
		pFullPath=malloc(strlen(cwd)+strlen(dir1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, dir1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(dir1, pFullPath+strlen(cwd)+1);
			}
			free(pFullPath);	
			pFullPath=NULL;				
		}
	}
	if(strlen(cwd) + strlen(dir1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long directory.");
		if (dir1 != dirname)
			free (dir1);			
		return;
	}
	if(access(dir1, F_OK)){
		control_printf(SL_FAILURE,"550 The folder doesn't exist.");
		if (dir1 != dirname)
			free (dir1);			
		return;
	}
	/* prepend path to filename */
	if (dir1[0] != '/'){
		strcat(cwd, "/");
		strcat(cwd, dir1);
	}
	else{
		strcpy(cwd, dir1);
	}
	if(rmdir(dir1)) {
#ifdef DEBUG		
		bftpd_log("Error: '%s' while trying to remove directory '%s'.\n",
				  strerror(errno), dirname);
#endif				  
		control_printf(SL_FAILURE, "451 %s.", strerror(errno));
	} else {
#ifdef DEBUG		
		bftpd_log("Removed directory '%s'.\n", dirname);
#endif		
		control_printf(SL_SUCCESS, "250 OK");
	}

	if(dir1!=dirname)
	     free(dir1);
	return;
}

void command_noop(char *params)
{
	control_printf(SL_SUCCESS, "200 OK");
}

void command_rnfr(char *oldname)
{
	FILE *phile;
	char *oldname1=NULL,cwd[MAX_FILE_LENGTH+1]={0};
	int ret,readonly=0;

#ifdef __USE_UNICODE__
	to_utf8(oldname);
#endif  
	ret=checkFolder(oldname,share,&readonly,NULL,1);
	switch(ret){
		case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			if(readonly)
				break;
		case 2:
			if(!readonly)
				control_printf(SL_FAILURE,"532 Permission denied.");		
			else
				control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}
	getcwd(cwd, sizeof(cwd) - 1);        
	if(!strcmp(cwd,"/")||oldname[0]=='/') {		
		oldname1=convertDir(oldname,share,NULL);
		if(!oldname1)
		oldname1=oldname;
		trans_dir(oldname1);	           
	}	        
	else{
		char *pFullPath=NULL;
			
		oldname1=oldname;
		pFullPath=malloc(strlen(cwd)+strlen(oldname1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, oldname1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(oldname1, pFullPath+strlen(cwd)+1);
			}
			free(pFullPath);	
			pFullPath=NULL;				
		}
	}   
	if(strlen(cwd) + strlen(oldname1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long source file or directory.");
		if (oldname1 != oldname)
			free (oldname1);		
		return;
	}			
	if(access(oldname1, F_OK)){
		control_printf(SL_FAILURE,"550 Source file or directory doesn't exist.");
		if (oldname1 != oldname)
			free (oldname1);				
		return;
	}  
		
	if ((phile = fopen(oldname1, "r"))) {
		fclose(phile);
		mystrncpy(philename, oldname1, sizeof(philename) - 1);
		state = STATE_RENAME;
		control_printf(SL_SUCCESS, "350 OK");
	} 
	else {
		control_printf(SL_FAILURE, "451 %s.", strerror(errno));
	}

	if(oldname1!=oldname)
		free(oldname1);
	return;
}

void command_rnto(char *newname)
{
	char *newname_bak=NULL, *newname1=NULL;
	char *philename1=NULL;
	int ret,readonly=0;
	char cwd1[MAX_FILE_LENGTH+1];
	int is_folder=0;
	struct stat f_stat;
	
#ifdef __USE_UNICODE__
	to_utf8(newname);
#endif 
	ret=checkFolder(newname,share,&readonly,NULL,1);
	switch(ret){
		case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			if(readonly)
				break;
		case 2:
			if(!readonly)
				control_printf(SL_FAILURE,"532 Permission denied.");		
			else
				control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}	
	getcwd(cwd1,sizeof(cwd1)-1);
	ret=-1;
	if(!strcmp(cwd1,"/")||newname[0]=='/'){         
		newname1=convertDir(newname,share,NULL);
		if(!newname1){
			newname1=newname;
			newname_bak=strdup(newname1);
		}
		ret=trans_dir(newname1);
	}
	else{
		char *pFullPath=NULL;
			
		newname1=newname;	
		pFullPath=malloc(strlen(cwd1)+strlen(newname1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd1, newname1);
			ret=trans_dir_with_prefix(cwd1, pFullPath);
			if(!ret){
				newname_bak=strdup(newname1);
				strcpy(newname1, pFullPath+strlen(cwd1)+1);
			}
			free(pFullPath);	
			pFullPath=NULL;				
		}			
	}		
	
	if(strlen(cwd1) + strlen(newname1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long target file or directory.");
		if (newname1 != newname)
			free (newname1);		
		return;
	}	
			
	if(philename && strcmp(philename, newname1) && !access(newname1, F_OK)){
		control_printf(SL_FAILURE,"550 Target file or directory already exists.");
		if (newname1 != newname)
			free (newname1);			
		return;	
	}
		
	if(!stat(philename, &f_stat) && S_ISDIR(f_stat.st_mode))
		is_folder=1;
		
	if(!strcmp(philename, newname1) && !ret){//Change the name back
		char *p0=NULL, *p1=NULL;
			
		if(newname_bak){
			p0=strrchr_m(newname_bak, '/');
			if(!p0)
				p0=newname_bak;				
		}
		else{
			p0=strrchr_m(newname, '/');
			if(!p0)
				p0=newname;
		}
		p1=strrchr_m(newname1, '/');
		if(!p1)
			p1=newname1;	
		strcpy(p1, p0);
	}
	if(newname_bak){
		free(newname_bak);
		newname_bak=NULL;
	}              
                                  
	if(rename(philename, newname1)) {
#ifdef DEBUG		
		bftpd_log("'%s' while trying to rename '%s' to '%s'.\n",
				  strerror(errno), philename, newname1);
#endif				  
		control_printf(SL_FAILURE, "451 %s.", strerror(errno));
	}
	else {
#ifdef DEBUG		
		bftpd_log("Successfully renamed '%s' to '%s'.\n", philename, newname1);
#endif		
		control_printf(SL_SUCCESS, "250 OK");
		state = STATE_AUTHENTICATED;
	}
  	if(is_folder)
  		chmod(newname1, 0777);
  	else
  		chmod(newname1, 0664);
	if(newname1!=newname)
	     free(newname1);
	if(philename1!=philename)
		free(philename1);  
	return;
}

void command_rest(char *params)
{
    offset = strtoull(params, NULL, 10);
	control_printf(SL_SUCCESS, "350 Restarting at offset %llu.", offset);
}

void command_size(char *filename)
{
	struct stat statbuf;
	char *filename1=NULL,cwd[MAX_FILE_LENGTH+1]={0};
	int ret;

#ifdef __USE_UNICODE__
	to_utf8(filename);
#endif 
	ret=checkFolder(filename,share,NULL,NULL,1);
	switch(ret){
		case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			break;
		case 2:
			control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}	
	getcwd(cwd,sizeof(cwd)-1);
	if(!strcmp(cwd,"/")||filename[0]=='/') {       
		filename1=convertDir(filename,share,NULL);
		if(!filename1)
			filename1=filename;
		trans_dir(filename1);			 	
	}			 
	else{
		char *pFullPath=NULL;
			
		filename1=filename;
		pFullPath=malloc(strlen(cwd)+strlen(filename1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, filename1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(filename1, pFullPath+strlen(cwd)+1);
			}
			free(pFullPath);	
			pFullPath=NULL;				
		}
	}		  
	if(strlen(cwd) + strlen(filename1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long file name.");
		if (filename1 != filename)
			free (filename1);		
		return;
	}			
	if(access(filename1, F_OK)){
		control_printf(SL_FAILURE,"550 The file doesn't exist.");
		if (filename1 != filename)
			free (filename1);			
		return;
	} 
           
	if (!lstat(filename1,&statbuf))
		control_printf(SL_SUCCESS, "213 %llu", (unsigned long long) statbuf.st_size);
	else
		control_printf(SL_FAILURE, "550 %s.", strerror(errno));

	if(filename1!=filename)
	     free(filename1);	
	return;	
}

void command_quit(char *params)
{
	control_printf(SL_SUCCESS, "221 %s", config_getoption("QUIT_MSG"));
	exit(0);
}

void command_stat(char *filename)
{
	char *filename1=NULL,cwd[MAX_FILE_LENGTH+1]={0};
	int ret;

#ifdef __USE_UNICODE__
	to_utf8(filename);
#endif 
	ret=checkFolder(filename,share,NULL,NULL,1);
	switch(ret){
		case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			break;
		case 2:
			control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}	
	getcwd(cwd,sizeof(cwd)-1);
	if(!strcmp(cwd,"/")||filename[0]=='/') {       
		filename1=convertDir(filename,share,NULL);
		if(!filename1)
			filename1=filename;
		trans_dir(filename1);			 	
	}			 
	else{
		char *pFullPath=NULL;
			
		filename1=filename;
		pFullPath=malloc(strlen(cwd)+strlen(filename1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, filename1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(filename1, pFullPath+strlen(cwd)+1);
			}
			free(pFullPath);	
			pFullPath=NULL;				
		}
	}		  
	if(strlen(cwd) + strlen(filename1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long file name.");
		if (filename1 != filename)
			free (filename1);		
		return;
	}			
	if(access(filename1, F_OK)){
		control_printf(SL_FAILURE,"550 The file doesn't exist.");
		if (filename1 != filename)
			free (filename1);			
		return;
	} 
	control_printf(SL_SUCCESS, "213-Status of %s:", filename);
	bftpd_stat(filename1, stderr, 1);
	control_printf(SL_SUCCESS, "213 End of Status.");
       
	if(filename1!=filename)
		free(filename1);	
	return;
}

/* SITE commands */

void command_chmod(char *params)
{
	int permissions;
	char filename[MAXCMD + 1],cwd[MAX_FILE_LENGTH+1]={0};
	char *filename1=NULL;
	int ret,readonly=0;
	
	strcpy(filename, strchr(params, ' ') + 1);
	*strchr(params, ' ') = '\0';
	sscanf(params, "%o", &permissions);
#ifdef __USE_UNICODE__
	to_utf8(filename);
#endif 
	ret=checkFolder(filename,share,&readonly,NULL,1);
       switch(ret){
       	case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			if(readonly)
			  break;
		case 2:
			control_printf(SL_FAILURE,"550 Permission denied.");
		return;
	}
	getcwd(cwd,sizeof(cwd)-1);
	if(!strcmp(cwd,"/")||filename[0]=='/') {       
		filename1=convertDir(filename,share,NULL);
		if(!filename1)
			filename1=filename;
		trans_dir(filename1);			 	
	}			 
	else{
		char *pFullPath=NULL;
			
		filename1=filename;
		pFullPath=malloc(strlen(cwd)+strlen(filename1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, filename1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(filename1, pFullPath+strlen(cwd)+1);
			}
			free(pFullPath);	
			pFullPath=NULL;				
		}
	}		  
	if(strlen(cwd) + strlen(filename1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long file name.");
		if (filename1 != filename)
			free (filename1);		
		return;
	}			
	if(access(filename1, F_OK)){
		control_printf(SL_FAILURE,"550 The file doesn't exist.");
		if (filename1 != filename)
			free (filename1);			
		return;
	} 
	if (chmod(filename1, permissions))
		control_printf(SL_FAILURE, "%s.", strerror(errno));
	else {
#ifdef DEBUG		
		bftpd_log("Changed permissions of '%s' to '%o'.\n", filename,
				  permissions);
#endif				  
		control_printf(SL_SUCCESS, "200 CHMOD successful.");
	}
	
	if(filename1!=filename)
	   free(filename1);
	return;
	
}

void command_chown(char *params)
{
	char foo[MAXCMD + 1], owner[MAXCMD + 1], group[MAXCMD + 1],filename[MAXCMD + 1];
	int uid, gid;
	char *filename1=NULL,cwd[MAX_FILE_LENGTH+1]={0};
	int ret,readonly=0;
		
	if (!strchr(params, ' ')) {
	     control_printf(SL_FAILURE, "550 Usage: SITE CHOWN <owner>[.<group>] <filename>");
	     return;
	}
	sscanf(params, "%[^ ] %s", foo, filename);
#ifdef __USE_UNICODE__
	to_utf8(filename);
#endif 
	ret=checkFolder(filename,share,&readonly,NULL,1);
	switch(ret){
		case 1:
			control_printf(SL_FAILURE,"550 No such file or directory.");
			return;	
		case 0:
			if(readonly)
				break;
		case 2:
			control_printf(SL_FAILURE,"550 Permission denied.");
			return;
	}	
	if (strchr(foo, '.'))
		sscanf(foo, "%[^.].%s", owner, group);
	else{
		strcpy(owner, foo);
		group[0] = '\0';
	}
	if (!sscanf(owner, "%i", &uid)){	// Is it a number?
	   if (((uid = mygetpwnam(owner, passwdfile))) < 0) {
		 control_printf(SL_FAILURE, "550 User '%s' not found.", owner);
		 return;
	   }
	}	  
	if (!sscanf(group, "%i", &gid)){
	   if (((gid = mygetpwnam(group, groupfile))) < 0) {
		 control_printf(SL_FAILURE, "550 Group '%s' not found.", group);
		 return;
		}
	}
	getcwd(cwd,sizeof(cwd)-1);
	if(!strcmp(cwd,"/")||filename[0]=='/') {       
		filename1=convertDir(filename,share,NULL);
		if(!filename1)
			filename1=filename;
		trans_dir(filename1);			 	
	}			 
	else{
		char *pFullPath=NULL;
			
		filename1=filename;
		pFullPath=malloc(strlen(cwd)+strlen(filename1)+2);
		if(pFullPath){
			sprintf(pFullPath, "%s/%s", cwd, filename1);
			ret=trans_dir_with_prefix(cwd, pFullPath);
			if(!ret){
				strcpy(filename1, pFullPath+strlen(cwd)+1);
			}
			free(pFullPath);	
			pFullPath=NULL;				
		}
	}		  
	if(strlen(cwd) + strlen(filename1) > MAX_FILE_LENGTH){
		control_printf(SL_FAILURE,"550 Too long file name.");
		if (filename1 != filename)
			free (filename1);		
		return;
	}			
	if(access(filename1, F_OK)){
		control_printf(SL_FAILURE,"550 The file doesn't exist.");
		if (filename1 != filename)
			free (filename1);			
		return;
	} 
	if (chown(filename1, uid, gid))
		control_printf(SL_FAILURE, "550 %s.", strerror(errno));
	else {
#ifdef DEBUG		
		bftpd_log("Changed owner of '%s' to UID %i GID %i.\n", filename, uid,
				  gid);
#endif				  
		control_printf(SL_SUCCESS, "200 CHOWN successful.");
	}
	
	if(filename1!=filename)
	   free(filename1);	
	return;
	
}

void command_site(char *str)
{
	control_printf(SL_FAILURE, "550 SITE commands are disabled.");
	return;
/*	const struct command subcmds[] = {
		{"chmod ", NULL, command_chmod, STATE_AUTHENTICATED},
		{"chown ", NULL, command_chown, STATE_AUTHENTICATED},
		{NULL, NULL, 0},
	};
	int i;
	if (!strcasecmp(config_getoption("ENABLE_SITE"), "no")) {
		control_printf(SL_FAILURE, "550 SITE commands are disabled.");
		return;
	}
	for (i = 0; subcmds[i].name; i++) {
		if (!strncasecmp(str, subcmds[i].name, strlen(subcmds[i].name))) {
			cutto(str, strlen(subcmds[i].name));
			subcmds[i].function(str);
			return;
		}
	}
	control_printf(SL_FAILURE, "550 Unknown command: 'SITE %s'.", str);
*/	
}

void command_auth(char *type)
{
    control_printf(SL_FAILURE, "550 Not implemented yet");
}

void command_utf8(char *type)
{
	if(!strcasecmp(type, "ON")){
#if 0		
		if(!client_noted){
			control_printf(SL_SUCCESS, "501 CLNT First");
			return;
		}
#endif		
		utf8_support=1;
	}
	else
		utf8_support=0;
    control_printf(SL_SUCCESS, "200 UTF-8 Encoding Enabled");
}

void command_client(char *type)
{
	if(strstr(type, "FileZilla"))
		file_zilla=1;
	client_noted=1;
    control_printf(SL_SUCCESS, "200 Noted");
}

/* Command parsing */
const struct command commands[] = {
	{"USER", "<sp> username", command_user, STATE_CONNECTED, 0},
	{"PASS", "<sp> password", command_pass, STATE_USER, 0},
	{"XPWD", "(returns cwd)", command_pwd, STATE_AUTHENTICATED, 1},
	{"PWD", "(returns cwd)", command_pwd, STATE_AUTHENTICATED, 0},
	{"TYPE", "<sp> type-code (A or I)", command_type, STATE_AUTHENTICATED, 0},
	{"PORT", "<sp> h1,h2,h3,h4,p1,p2", command_port, STATE_AUTHENTICATED, 0},
	{"EPRT", "<sp><d><net-prt><d><ip><d><tcp-prt><d>", command_eprt, STATE_AUTHENTICATED, 1},
	{"PASV", "(returns address/port)", command_pasv, STATE_AUTHENTICATED, 0},
	{"EPSV", "(returns address/post)", command_epsv, STATE_AUTHENTICATED, 1},
	{"ALLO", "<sp> size", command_allo, STATE_AUTHENTICATED, 1},
	{"STOR", "<sp> pathname", command_stor, STATE_AUTHENTICATED, 0},
	{"APPE", "<sp> pathname", command_appe, STATE_AUTHENTICATED, 1},
	{"RETR", "<sp> pathname", command_retr, STATE_AUTHENTICATED, 0},
	{"LIST", "[<sp> pathname]", command_list, STATE_AUTHENTICATED, 0},
	{"NLST", "[<sp> pathname]", command_nlst, STATE_AUTHENTICATED, 0},
	{"SYST", "(returns system type)", command_syst, STATE_CONNECTED, 0},
	{"MDTM", "<sp> pathname", command_mdtm, STATE_AUTHENTICATED, 1},
	{"XCWD", "<sp> pathname", command_cwd, STATE_AUTHENTICATED, 1},
	{"CWD", "<sp> pathname", command_cwd, STATE_AUTHENTICATED, 0},
	{"XCUP", "(up one directory)", command_cdup, STATE_AUTHENTICATED, 1},
	{"CDUP", "(up one directory)", command_cdup, STATE_AUTHENTICATED, 0},
	{"DELE", "<sp> pathname", command_dele, STATE_AUTHENTICATED, 0},
	{"XMKD", "<sp> pathname", command_mkd, STATE_AUTHENTICATED, 1},
	{"MKD", "<sp> pathname", command_mkd, STATE_AUTHENTICATED, 0},
	{"XRMD", "<sp> pathname", command_rmd, STATE_AUTHENTICATED, 1},
	{"RMD", "<sp> pathname", command_rmd, STATE_AUTHENTICATED, 0},
	{"NOOP", "(no operation)", command_noop, STATE_AUTHENTICATED, 0},
	{"RNFR", "<sp> pathname", command_rnfr, STATE_AUTHENTICATED, 0},
	{"RNTO", "<sp> pathname", command_rnto, STATE_RENAME, 0},
	{"REST", "<sp> byte-count", command_rest, STATE_AUTHENTICATED, 1},
	{"SIZE", "<sp> pathname", command_size, STATE_AUTHENTICATED, 1},
	{"QUIT", "(close control connection)", command_quit, STATE_CONNECTED, 0},
	{"HELP", "[<sp> command]", command_help, STATE_AUTHENTICATED, 0},
	{"STAT", "<sp> pathname", command_stat, STATE_AUTHENTICATED, 0},
	{"SITE", "<sp> string", command_site, STATE_AUTHENTICATED, 0},
	{"FEAT", "(returns list of extensions)", command_feat, STATE_AUTHENTICATED, 0},
 //       {"AUTH", "<sp> authtype", command_auth, STATE_CONNECTED, 0},
//        {"ADMIN_LOGIN", "(admin)", command_adminlogin, STATE_CONNECTED, 0},
	{"OPTS UTF8", "<sp> pathname", command_utf8, STATE_AUTHENTICATED, 0},
	{"UTF8", "(no operation)", command_utf8, STATE_AUTHENTICATED, 1},
	{"CLNT", "<sp> string", command_client, STATE_AUTHENTICATED, 1},
	{NULL, NULL, NULL, 0, 0}
};

void command_feat(char *params)
{
    int i;
    control_printf(SL_SUCCESS, "211-Extensions supported:");
    for (i = 0; commands[i].name; i++)
        if (commands[i].showinfeat)
            control_printf(SL_SUCCESS, " %s", commands[i].name);
    control_printf(SL_SUCCESS, "211 End");
}

void command_help(char *params)
{
	int i;
	if (params[0] == '\0') {
		control_printf(SL_SUCCESS, "214-The following commands are recognized.");
		for (i = 0; commands[i].name; i++)
			control_printf(SL_SUCCESS, "214-%s", commands[i].name);
        control_printf(SL_SUCCESS, "214 End of help");
	} else {
		for (i = 0; commands[i].name; i++)
			if (!strcasecmp(params, commands[i].name))
				control_printf(SL_FAILURE, "214 Syntax: %s", commands[i].syntax);
	}
}

int parsecmd(char *str)
{
	int i;
	char *p, *pp, confstr[18]; /* strlen("ALLOWCOMMAND_XXXX") + 1 == 18 */
	char param_str[3*MAX_FILE_LENGTH+1]={0};
	
	p = pp = str;			/* Remove garbage in the string */
	while (*p)
		if ((unsigned char) *p < 32)
			p++;
		else
			*pp++ = *p++;
	*pp++ = 0;
	for (i = 0; commands[i].name; i++) {	/* Parse command */
	     if (!strncasecmp(str, commands[i].name, strlen(commands[i].name))) {
                   sprintf(confstr, "ALLOWCOMMAND_%s", commands[i].name);
                   if (!strcasecmp(config_getoption(confstr), "no")) {
                        control_printf(SL_FAILURE, "550 The command '%s' is disabled.",
                        commands[i].name);
                        return 1;
                   }
			cutto(str, strlen(commands[i].name));
			p = str;
			while ((*p) && ((*p == ' ') || (*p == '\t')))
				p++;
			memmove(str, p, strlen(str) - (p - str) + 1);
			memset(param_str, '\0', sizeof(param_str));
			if (state >= commands[i].state_needed) {
				strcpy(param_str, str);
//				bftpd_log("%s: param_str=%s\n",__FUNCTION__, param_str);
				commands[i].function(param_str);
				return 0;
			}
			else {
				switch (state) {
					case STATE_CONNECTED: {
						control_printf(SL_FAILURE, "503 USER expected.");
						return 1;
					}
					case STATE_USER: {
						control_printf(SL_FAILURE, "503 PASS expected.");
						return 1;
					}
					case STATE_AUTHENTICATED: {
						control_printf(SL_FAILURE, "503 RNFR before RNTO expected.");
						return 1;
					}
				}
			}
	     }
	}
	control_printf(SL_FAILURE, "500 Unknown command: \"%s\"", str);
	return 0;
}

