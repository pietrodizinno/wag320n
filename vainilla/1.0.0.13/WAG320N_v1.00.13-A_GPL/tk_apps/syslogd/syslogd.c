/* vi: set sw=4 ts=4: */
/*
 * Mini syslogd implementation for busybox
 *
 * Copyright (C) 1999,2000 by Lineo, inc. and Erik Andersen
 * Copyright (C) 1999,2000,2001 by Erik Andersen <andersee@debian.org>
 *
 * Copyright (C) 2000 by Karl M. Hegbloom <karlheg@debian.org>
 *
 * "circular buffer" Copyright (C) 2001 by Gennady Feldman <gfeldman@cachier.com>
 *
 * Maintainer: Gennady Feldman <gena01@cachier.com> as of Mar 12, 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <paths.h>
#include <signal.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include "sysklogd.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <nvram.h>
/* SYSLOG_NAMES defined to pull some extra junk from syslog.h */
#define SYSLOG_NAMES
#include <sys/syslog.h>
#include <sys/uio.h>
#include <net/if.h>
#include <sys/ioctl.h>



/* Path for the file where all log messages are written */
#define __LOG_FILE "/var/log/messages"
#define __CONF_FILE "/etc/syslog.conf"
#define SHOW_HOSTNAME

/* Path to the unix socket */
static char lfile[MAXPATHLEN]="";

static char *logFilePath = __LOG_FILE;

#define dprintf(msg,...)
struct syslog_conf conf;

#define ALERT_MAX_INTERVAL 3*60
static time_t last_send_mail=0;

#define	ROTATE_NUM			4
static int logFileMaxSize = 10*1024;

/* interval between marks in seconds */
static int MarkInterval = 10 * 60;

#ifdef SHOW_HOSTNAME
/* localhost's name */
static char LocalHostName[256]="";
#endif
#ifdef _WIRELESS_
#define LOGIC_LAN_IFNAME  "br0"
#else
#define LOGIC_LAN_IFNAME  "eth0"
#endif

#ifdef BB_FEATURE_REMOTE_LOG
#include <netinet/in.h>
/* udp socket for logging to remote host */
static int remotefd = -1;
/* where do we log? */
static char *RemoteHost=NULL;
/* what port to log to? */
static int RemotePort = 514;
/* To remote log or not to remote log, that is the question. */
static int doRemoteLog = FALSE;
static int local_logging = FALSE;
#endif

static int email_send_threshold = 0;
#define MAXLINE         1024            /* maximum line length */


/* circular buffer variables/structures */
#ifdef BB_FEATURE_IPC_SYSLOG
#if __GNU_LIBRARY__ < 5
#error Sorry.  Looks like you are using libc5.
#error libc5 shm support isnt good enough.
#error Please disable BB_FEATURE_IPC_SYSLOG
#endif

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

/* our shared key */
static const long KEY_ID = 0x414e4547; /*"GENA"*/

// Semaphore operation structures
static struct shbuf_ds {
    int num;                // number of message
    int size;               // size of data written
    int head;               // start of message list
    int tail;               // end of message list
    /* can't use char *data */
    char data[1];           // data/messages
} *buf = NULL;                  // shared memory pointer

static struct sembuf SMwup[1] = {{1, -1, IPC_NOWAIT}}; // set SMwup
static struct sembuf SMwdn[3] = {{0, 0}, {1, 0}, {1, +1}}; // set SMwdn

static int      shmid = -1;     // ipc shared memory id
static int      s_semid = -1;   // ipc semaphore id
int     data_size = 50000; // data size
int     shm_size = 50000 + sizeof(*buf); // our buffer size
static int circular_logging = FALSE;

/* Ron */
static void clear_signal(int sig);
static void reload_signal(int sig);
static char last_log[1024]="";

int mylog(const char *format, ...)
{
    va_list args;
    FILE *fp;

    fp = fopen("/var/glb_dbg", "a+");

    if (!fp) {
        fprintf(stderr, "fp is NULL\n");
	    return -1;
    }

    va_start(args, format);
    vfprintf(fp, format, args);
    va_end(args);

    fclose(fp);
    return 0;
}





void logMessage (int pri, char *msg);
/*
 * sem_up - up()'s a semaphore.
 */
static inline void sem_up(int semid)
{
    if ( semop(semid, SMwup, 1) == -1 )
        perror_msg_and_die("semop[SMwup]");
}

/*
 * sem_down - down()'s a semaphore
 */
static inline void sem_down(int semid)
{
    if ( semop(semid, SMwdn, 3) == -1 )
        perror_msg_and_die("semop[SMwdn]");
}


void ipcsyslog_cleanup(void){
    dprintf("Exiting Syslogd!\n");
    if (shmid != -1)
        shmdt(buf);

    if (shmid != -1)
        shmctl(shmid, IPC_RMID, NULL);
    if (s_semid != -1)
        semctl(s_semid, 0, IPC_RMID, 0);
}

void ipcsyslog_init(void){
    if (buf == NULL){
        if ((shmid = shmget(KEY_ID, shm_size, IPC_CREAT | 1023)) == -1)
            perror_msg_and_die("shmget");


        if ((buf = shmat(shmid, NULL, 0)) == (void *)-1)
            perror_msg_and_die("shmat");


        buf->size=data_size;
        buf->num=buf->head=buf->tail=0;

        // we'll trust the OS to set initial semval to 0 (let's hope)
        if ((s_semid = semget(KEY_ID, 2, IPC_CREAT | IPC_EXCL | 1023)) == -1){
            if (errno == EEXIST){
                if ((s_semid = semget(KEY_ID, 2, 0)) == -1)
                    perror_msg_and_die("semget");
            }else
                perror_msg_and_die("semget");
        }
    }else{
        dprintf("Buffer already allocated just grab the semaphore?");
    }
}

static void send_mail_signal(int sig)
{
    //	sem_down(s_semid);

    if(conf.mail_enable==1){
        char cmd[1024];
        char auth_str[300]={0,};
/**
        if((*conf.mail_auth_enable == '1') && *conf.mail_auth_user && *conf.mail_auth_pass) {
            sprintf(auth_str, " -U %s -P %s", conf.mail_auth_user, conf.mail_auth_pass);
        }
        else {
            auth_str[0] = '\0';
        }
**/
        sprintf(cmd,"/usr/sbin/smtpc -m -h %s -r %s -f %s -s \"%s\"%s </var/log/messages "
                ,conf.mail_server
                ,conf.mail_receiver
                ,conf.mail_sender
                ,conf.mail_subject
                ,auth_str);

        system(cmd);
        buf->head=0;
        buf->tail=0;
        buf->num=0;
//        unlink("/var/log/messages");
        time(&last_send_mail);
    }

    //	sem_up(s_semid);
}

/* write message to buffer */
void circ_message(const char *msg){
    int l=strlen(msg); /* count the whole message w/ '\0' included */

    sem_down(s_semid);

    buf->num++;
    if ( (buf->tail + l) < buf->size ){
        if ( buf->tail < buf->head){
            if ( (buf->tail + l) >= buf->head ){
                int k= buf->tail + l - buf->head;
                char *c=memchr(buf->data+buf->head + k,'\n',buf->size - (buf->head + k));
                buf->head=(c != NULL)?( c - buf->data + 1):0;

            }
        }
        strncpy(buf->data + buf->tail,msg,l); /* append our message */
        buf->tail+=l;
    }else{
        char *c;
        int k=buf->tail + l - buf->size;

        c=memchr(buf->data + k ,'\n', buf->size - k);

        if (c != NULL) {
            buf->head=c-buf->data+1;
            strncpy(buf->data + buf->tail, msg, l - k - 1);
            strcpy(buf->data, &msg[l-k-1]);
            buf->tail = k + 1;
        }else{
            buf->head = buf->tail = 0;
        }

    }
    sem_up(s_semid);
}
#endif  /* BB_FEATURE_IPC_SYSLOG */

/* try to open up the specified device */
int device_open(char *device, int mode)
{
    int m, f, fd = -1;

    m = mode | O_NONBLOCK;

    /* Retry up to 5 times */
    for (f = 0; f < 5; f++)
        if ((fd = open(device, m, 0600)) >= 0)
            break;
    if (fd < 0)
        return fd;
    /* Reset original flags. */
    if (m != mode)
        fcntl(fd, F_SETFL, mode);
    return fd;
}
int vdprintf(int d, const char *format, va_list ap)
{
    char buf[BUF_SIZE];
    int len;

    len = vsnprintf(buf, sizeof(buf), format, ap);
    return write(d, buf, len);
}

enum {
    NEEDNOT_DST,    /*need not daylight saving time*/
    NA_DST,         /*North America need DST*/
    EU_DST,         /*Europe DST*/
    CHILE_DST,		/*Chile dst*/
    SA_DST,			/* sourth america,for example: Brazil*/
    IRAQ_DST,		/* Iraq and Iran */
    AU2_DST,		/*Australia - Tasmania*/
    AU3_DST,		/*New Zealand, Chatham */
    AF_DST,		/* Egypt*/
    AU_DST,          /*Australia DST*/
};

/*int checkDayLight(int tmz, struct tm *st) {
    int dst_region;
    int last_left_day = st->tm_yday;
    int current_year = st->tm_year+1900;
    int startDay=0;
    int endDay=0 ;
    int bSaveFlag = 0;
	char *pt = nvram_safe_get("time_zone");

    switch(tmz){
        case 3:case 4:case 6:case 7:case 9:case 10:case 13:case 15:case 18:
            dst_region = NA_DST;
            break;
        case 17:
            dst_region = CHILE_DST;
            break;
        case 19:
            dst_region = SA_DST;
            break;
        case 21:case 22:case 23:case 26:case 27:case 28:case 29:case 30:case 32:
        case 33:case 36:case 40:case 44:case 46:case 50:case 55:case 57:case 63:case 70:
            dst_region = EU_DST;
            break;
        case 34:
            dst_region = AF_DST;
            break;
        case 38:case 42:
            dst_region = IRAQ_DST;
            break;
        case 64:case 67:
            dst_region = AU_DST;
            break;
        case 69:
            dst_region = AU2_DST;
            break;
        case 72:
            dst_region = AU3_DST;
            break;
        default:
            return 0;
    }

    switch(dst_region) {
        case NA_DST:
            startDay=97-((current_year-2002+((current_year-2000)/4))%7);
            endDay=304-(current_year-1999+((current_year-1996)/4))%7;
            //Daylight Saving Time in the United States and Canaga are changeing.
	//The period of Daylingt Savings Time will be a total of 4 weeks longer than 
	//before - Beginning 3 weeks earlier in Spring and endig 1 week later in the Fall.
		if(strncmp(pt + 3, "-8", 2) == 0 && current_year >= 2007) // GMT-8
        {
        	startDay=73  - (current_year- 1999 +(current_year - 1997)/4)%7;
           	endDay=311   - (current_year- 1999 +(current_year - 1997)/4)%7;
		}
 
	    break;
        case EU_DST:
            startDay=89-(current_year-2002+((current_year-2000)/4))%7;
            endDay=303-(current_year-1999+((current_year-1996)/4))%7;
            break;
        case AU_DST:
            startDay=303-(current_year-1999+((current_year-1996)/4))%7;
            endDay=89-(current_year-2002+((current_year-2000)/4))%7;
            break;
        case CHILE_DST:
            startDay = 286 - (current_year-2000+((current_year-2000)/4))%7;
            endDay = 72 - (current_year-1998+((current_year-1996)/4))%7;
            break;
        case SA_DST:
            startDay = 310 - (current_year-2004+((current_year-2004)/4))%7;
            endDay = 51 - (current_year-1999+((current_year-1996)/4))%7;
            break;
        case AF_DST:
            startDay = 119 - (current_year-2004+((current_year-2004)/4))%7;
            endDay = 272 - (current_year-2004+((current_year-2004)/4))%7;
            break;
        case IRAQ_DST:
            if (current_year%4 != 0) {
                startDay = 90;
                endDay = 273;
            }
            else {
                startDay = 91;
                endDay = 274;
            }
            break;
        case AU2_DST:
            startDay = 279 - (current_year-2001+((current_year-2000)/4))%7;
            endDay = 89 - (current_year-2002+((current_year-2000)/4))%7;
            break;
        case AU3_DST:
            startDay = 279 - (current_year-2001+((current_year-2000)/4))%7;
            endDay = 79 - (current_year-1999+((current_year-1996)/4))%7;
            break;
        default:
            break;
    }

    if (startDay > endDay) {
        if ((last_left_day >= startDay)||(last_left_day <= endDay))
            bSaveFlag = 1;
        else
            bSaveFlag = 0;
    }
    else {
        if ((last_left_day >= startDay)&&(last_left_day <= endDay))
            bSaveFlag = 1;
        else
            bSaveFlag = 0;
    }

    return  bSaveFlag;

}*/

//int time_adjust(char *time_mode, char *tz, char *time_daylight) {
//
//    int hour;
//    int min;
//    int index;
//    int time_adj;
//    time_t t;
//    struct tm *st;
//    int daylight;
//
//    if(*time_mode == '1') {
//        return 0;
//    }
//
//    if(time_daylight && *time_daylight == '1') {
//        daylight = 1;
//    }
//    else {
//        daylight = 0;
//    }
//
//    sscanf(tz+3, "%d %d %d", &hour, &min, &index);
//    time_adj = hour*3600 + min * 60;
//    if(!daylight) {
//        return time_adj;
//    }
//    index--; /* index range: [0 -- 74] */
//
//    time(&t);
//    t += time_adj;
//    st = localtime(&t);
//
//    if(checkDayLight(index, st)) {
//        time_adj += 3600;
//    }
//
//    return time_adj;
//}

/* Note: There is also a function called "message()" in init.c */
/* Print a message to the log file. */
static void message (char *fmt, ...) __attribute__ ((format (printf, 1, 2)));
static void message (char *fmt, ...)
{
    int fd=0, i=0;
    struct flock fl;
    va_list arguments;
	struct stat st;
	char file_name_0[128]={0},file_name_1[128]={0};
	
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 1;
#ifdef BB_FEATURE_IPC_SYSLOG
    if ((circular_logging == TRUE) && (buf != NULL)){
        char b[1024];
        va_start (arguments, fmt);
        vsnprintf (b, sizeof(b)-1, fmt, arguments);
        va_end (arguments);
        circ_message(b);
#ifdef DEBUG
        printf("head=%d tail=%d\n",buf->head,buf->tail);
#endif
        /* print_circ_buf */
        if((fd=open(logFilePath,O_WRONLY| O_CREAT|O_TRUNC|O_NONBLOCK))<0)
            return;
        fl.l_type = F_WRLCK;
        fcntl(fd, F_SETLKW, &fl);
        if(buf->tail > buf->head){
            write(fd,buf->data,buf->tail);
            write(fd,"\0",1);
        }else {

            write(fd,buf->data+buf->head,buf->size-buf->head-1);
            write(fd,buf->data,buf->tail);
            write(fd,"\0",1);

            if(conf.mail_log_full==1)
                send_mail_signal(0);

        }
/**
        if(buf->num >= conf.email_qlen) {
            send_mail_signal(0);
        }
**/
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLKW, &fl);
        close(fd);
    }
    else
#endif
        if ((fd = device_open (logFilePath, O_WRONLY | O_CREAT | O_NOCTTY | O_APPEND | O_NONBLOCK)) >= 0) {
            fl.l_type = F_WRLCK;
            fcntl (fd, F_SETLKW, &fl);
            va_start (arguments, fmt);
            vdprintf (fd, fmt, arguments);
            va_end (arguments);
            fl.l_type = F_UNLCK;
            fcntl (fd, F_SETLKW, &fl);
            close (fd);
			if (stat(logFilePath, &st) != -1){
				if(st.st_size >= logFileMaxSize){
					for(i=ROTATE_NUM; i>=0; i--){	
						if(i==0)
							sprintf(file_name_0, "%s", logFilePath);
						else
							sprintf(file_name_0, "%s.%d", logFilePath,i);
						if(i==ROTATE_NUM)
							remove(file_name_0);
						else{
							sprintf(file_name_1, "%s.%d", logFilePath,i+1);
							rename(file_name_0, file_name_1);							
						}
					}
				}
			}
        }
        else {
            /* Always send console messages to /dev/console so people will see them. */
            if ((fd = device_open (_PATH_CONSOLE, O_WRONLY | O_NOCTTY | O_NONBLOCK)) >= 0) {
                va_start (arguments, fmt);
                vdprintf (fd, fmt, arguments);
                va_end (arguments);
                close (fd);
            }
            else {
                fprintf (stderr, "Bummer, can't print: ");
                va_start (arguments, fmt);
                vfprintf (stderr, fmt, arguments);
                fflush (stderr);
                va_end (arguments);
            }
        }
}
/**
inline int check_level(int pri) {
    return conf.log_level[pri&0x07];
}
**/
int check_log(int fac)
{
    int i=0;
    while(conf.log_list[i]>=0 && i < 20){
        if(conf.log_list[i++]==fac)
            return TRUE;
    }
    return FALSE;
}
#if 0
void strccpy(char *dst, char *src,char c)
{
    char *pt=src;
    if(pt==NULL){
        printf("pt==NULL");
        dst[0]='\0';
        return ;
    }
    for(;*pt!=c && *pt!='\0';*dst++=*pt++);
    *dst='\0';

}
#endif
void strccpy2(char *dst, char *src,char *key,char c)
{
    char *pt=strstr(src,key);
    if(pt==NULL){
        dst[0]='\0';
        return ;
    }
    pt+=strlen(key);
    for(;*pt!=c && *pt!='\0';*dst++=*pt++);
    *dst='\0';

}
/*
 * check if msg is only white space
 */
#define is_whitespace(c)    (((c)==' ') || ((c) == '\t') || ((c) == '\n') )

static int white_msg(char *msg) {
    char *p = msg;

    for(; *p; p++) {
        if(!is_whitespace(*p)) {
            return 0;
        }
    }
    return 1;
}

void logMessage (int pri, char *msg)
{
    time_t now;
    struct tm *st;
    char timestamp[32]="";
    char res[20] = "";
    CODE *c_fac;
    int fac=0;
    int send_mail=email_send_threshold;
    int do_log=1;
//    char *p;
    char *wday[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

//    mylog("[%s():%d] pri is <%d> msg is <%s>\n", __FUNCTION__, __LINE__, pri, msg);

//    if(!check_level(pri)) {
//        return;
//    }
//	mylog("in the logMessage function step 1");
    if (pri!=0){
        for (c_fac = facilitynames;
                c_fac->c_name && !(c_fac->c_val == LOG_FAC(pri) << 3); c_fac++);
        fac=c_fac->c_val;
        //for (c_pri = prioritynames;
        //	c_pri->c_name && !(c_pri->c_val == LOG_PRI(pri)); c_pri++);
    }

    /* check this log  */
    //	if(check_log(fac)==FALSE)
    //		return;

    /* if msg had time stamp ,remove it*/
    if (strlen(msg) > 16 && msg[3] == ' ' && msg[6] == ' ' &&
            msg[9] == ':' && msg[12] == ':' && msg[15] == ' ')
        msg+=16;

    /* we don't need log from pppd */
    if(strstr(msg,"ppp0 -> "))
        return;
//	mylog("in the logMessage function step 2");
#ifdef DEBUG_CALL_TRACE
    /* log Call Trace for debug */
    if(strstr(msg,"Call Trace")){
        //message("<DEBUG> - %s\n",msg);
        system("/bin/dmesg > /var/log/messages");
        exit(0);
    }
#endif
//	mylog("in the logMessage function step 3");
    // add by john,  format pluto log message
    if (strstr(msg,"pluto")!=NULL){
        // filter chars before :, add header [VPN log]
        char *msgP=msg;
        while(*msgP!='\0' && *msgP++!=':');  // filter chars before :
        if (strncmp(msgP," |",2)==0)   //  if begin with |, it is debug message,skip it
            return;
        sprintf(msg,"[VPN Log]:%s",msgP);

        goto add_timestamp;
    }
    // end of add by john

//	mylog("in the logMessage function step 4");

    while(*msg!='\0' && *msg++!=':');

    if(white_msg(msg)) {
        return;
    }
//	mylog("in the logMessage function step 5");
    /* =======================================================
     * After record [Firewall Log-...] log msg,
     * the follow msg reveive from socket sometimes error,
     * so I drop it here -- Jeff -- Apr.4.2005 --
     * ======================================================= */
    if(strstr(last_log,"[Firewall Log-")!=NULL && strstr(msg,"[Firewall Log-")==NULL
            && ((strstr(msg,"ID=")!=NULL && strstr(msg,"SEQ=")!=NULL)||(strstr(msg,"URGP=")!=NULL)))
        return;
//	mylog("in the logMessage function step 6");
    /* ==============================================================================
     * NOTICE: -- Jeff add here -- Apr.4.2005 --
     *
     * Description:
     *      In the previous --  " if(pri!=0){
     * 		    for (c_fac = facilitynames;
     *              ......} "
     *      LOG_FAC(p) defined as (((p) & 0x03F8) >> 3),but we get "pri" there
     *      is from LOG_EMERG(0) to LOG_DEBUG(7),then the value of LOG_FAC(pri)
     *      is always ZERO,and "fac" equals ZERO too.
     *      In the following part of "handle kern message",LOG_LOCAL0 defined
     *      as (16<<3),so the "fac(0)" will never equals to "LOG_LOCAL0(128)",
     *
     * Result:
     *      It will never into if(){......}. So the msg which include log_keyword
     *      or mail_keyword will never be handled to a correct format.
     *
     * Solution:
     *      The msg which include log_keyword or mail_keyword always include '['
     *      and ']', So we broaden the condition for accepting these msgs.
     * ============================================================================== */
    /* handle kern message*/
    //	if(fac==LOG_LOCAL0){
    if(fac==LOG_LOCAL0 || strchr(msg,'[')!=NULL || strchr(msg,']')!=NULL){

        char proto[8]="",src[16]="",dst[128]="",spt[8]="",dpt[8]="";
        char prefix[64]="";
        char *pt;
        char *action;
        int i;

//        mylog("[%s():%d] msg is <%s>\n", __FUNCTION__, __LINE__, msg);
        if(((pt=strchr(msg,'['))==NULL) || (strchr(msg,']')==NULL))
            return;

        for(i=0,pt;*pt!=']';prefix[i++]=*pt++);
        prefix[i++]=']';
        prefix[i]='\0';

        if(strstr(conf.mail_keyword,prefix)){
            email_send_threshold++;
        }
		if(strstr(conf.log_keyword,prefix)==NULL){
			do_log=0;
		}
		if(strstr(prefix,"match")){
			do_log=1;
		}

        strccpy2(proto,msg,"PROTO=",0x20);
        if(strcmp(proto,"47")==0){
            strcpy(proto,"GRE");
        }else if(strcmp(proto,"50")==0){
            strcpy(proto,"ESP");
        }else if(strcmp(proto,"51")==0){
            strcpy(proto,"AH");
        }else if(strcmp(proto,"115")==0){
            strcpy(proto,"L2TP");
        }

        strccpy2(src,msg,"SRC=",0x20);
        strccpy2(dst,msg,"DST=",0x20);
        strccpy2(spt,msg,"SPT=",0x20);
        strccpy2(dpt,msg,"DPT=",0x20);
#if 0
        if(strstr(prefix,"Access Log-")!=NULL){
            struct hostent *host;
            struct in_addr addr;
            addr.s_addr=inet_addr(dst);
            host=gethostbyaddr((char *)&addr,4,AF_INET);
            if(host){
                strcpy(dst,host->h_name);
            }
        }
#endif
        if(!memcmp(prefix, "[Access Log", sizeof("[Access Log")-1)) {
            action = strchr(msg, ']')+1;
            switch(*action) {
            case 'A':   /* Allow */
                strcat(prefix, " Allow");
                break;
            case 'D':   /* Deny */
                strcat(prefix, " Deny");
                break;
            case 'I':   /* Incoming */
                strcat(prefix, "I");
                break;
            case 'O':   /* Outgoing */
                strcat(prefix, "O");
                break;
            default:
                break;
            }
        }
        pt=msg;
        if(spt[0]!='\0' && strcmp(prefix,"[Firewall Log-PORT SCAN]")!=0)
            sprintf(pt,"%s %s Packet - %s:%s --> %s:%s"
                    ,prefix, proto, src, spt, dst, dpt);
        else
            sprintf(pt,"%s %s Packet - %s --> %s"
                    ,prefix, proto, src, dst);
    }else
        msg++;
//	mylog("in the logMessage function step 7");
#if 1
    /* dont know the meaning of this  code, comment it. -- Argon Cheng */
    if(strcmp(last_log,msg)==0){
        //if mail_keyword match,do check whether send mail,but don't log it.
        if(conf.mail_enable==1 && email_send_threshold>send_mail)
            do_log=0;
        else
            return ;
    }else{
        strcpy(last_log,msg);
    }
#else
    strcpy(last_log, msg);
#endif


add_timestamp:
    /* time stamp */
    time(&now);
    //now += time_adjust(conf.time_mode, conf.TZ, conf.daylight);
    st=localtime(&now);
#if 1
    sprintf(timestamp,"%s, %d-%02d-%02d %02d:%02d:%02d"
            ,wday[st->tm_wday]
            ,st->tm_year+1900
            ,st->tm_mon+1
            ,st->tm_mday
            ,st->tm_hour
            ,st->tm_min
            ,st->tm_sec);
#else
    memset(timestamp, 0, sizeof(timestamp));
    asctime_r(st, timestamp); /* "Wed Jun 30 21:33:44 1987\n" */

    memmove(timestamp, timestamp+4, strlen(timestamp)-3); /* "Jun 30 21:33:44 1987\n" */
    p = strrchr(timestamp, ' ');
    if(p) {
        p++;
        *p = '\0';
    }/* "Jun 30 21:33:44 " */
#endif
//	mylog("in the logMessage function step 7");
    if(conf.mail_enable==1){
        char cmd[1024];
        FILE *fp;
        //sem_down(s_semid);
        if(email_send_threshold<=atoi(conf.dos_thresholds)){//record msg,the max number of the msg is dos_thresholds.
            if(email_send_threshold>send_mail){//if msg match mail_keyword,no match no record.
                fp=fopen("/var/log/alert","a+");

                if(fp==NULL)
                    return ;
                fprintf(fp,"No.%03d  %s - %s\n",email_send_threshold, timestamp, msg);
                fclose(fp);
            }
        }
        if(email_send_threshold>=atoi(conf.dos_thresholds)){
            if((now-last_send_mail)>ALERT_MAX_INTERVAL){
                sprintf(cmd,"/usr/sbin/smtpc -m -h %s -r %s -f %s -s \"%s\" </var/log/alert "
                        ,conf.mail_server
                        ,conf.mail_receiver
                        ,conf.mail_sender
                        ,conf.mail_subject_alert);

                system(cmd);
                time(&last_send_mail);
                email_send_threshold=0;
                send_mail=0;
                unlink("/var/log/alert");
            }
        }
        //sem_up(s_semid);
    }
    else
        email_send_threshold=0;

//	mylog("in the logMessage function step 8");
    memset(&res, 0, sizeof(res));
    snprintf(res, sizeof(res), "<%d>", pri);
    /* todo: supress duplicates */
#ifdef BB_FEATURE_REMOTE_LOG
    /* send message to remote logger */
    if ( (-1 != remotefd) && (doRemoteLog==TRUE)){
        static const int IOV_COUNT = 6;
        struct iovec iov[IOV_COUNT];
        struct iovec *v = iov;
        char *space = " ";

        v->iov_base = res ;
        v->iov_len = strlen(res);
        v++;

        v->iov_base = timestamp ;
        v->iov_len = strlen(timestamp);
        v++;
        
        v->iov_base = space ;
        v->iov_len = strlen(space);
        v++;
        
        v->iov_base = LocalHostName ;
        v->iov_len = strlen(LocalHostName);
        v++;

        v->iov_base = space ;
        v->iov_len = strlen(space);
        v++;

        v->iov_base = msg;
        v->iov_len = strlen(msg);
writev_retry:
        if ( -1 == writev(remotefd,iov, IOV_COUNT)){
            if (errno == EINTR) goto writev_retry;
            error_msg_and_die("cannot write to remote file handle on"
                    "%s:%d",RemoteHost,RemotePort);
        }
    }
#endif
//	mylog("in the logMessage function step 9");
    if(do_log==0 || conf.log_enable ==0)
        return ;
//	mylog("in the logMessage function step 10");
#if 0 //def SHOW_HOSTNAME
    /* now spew out the message to wherever it is supposed to go */
    message("%s %s %s %s\n", timestamp, LocalHostName, res, msg);
#else
    message("%s - %s\n", timestamp, msg);
#endif
}

static void quit_signal(int sig)
{
    //logMessage(LOG_SYSLOG | LOG_INFO, "System log daemon exiting.");
    unlink(lfile);
#ifdef BB_FEATURE_IPC_SYSLOG
    ipcsyslog_cleanup();
#endif
    exit(TRUE);
}
static int log_start=0;

static inline void router_start()
{
    struct sysinfo info;
    time_t now;
    char *wday[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    struct tm *st;
    char timestamp[128];

    if(log_start==1)
        return ;

    sysinfo(&info);
    /* time stamp */
    time(&now);
    //now += time_adjust(conf.time_mode, conf.TZ, conf.daylight);
    now-=info.uptime;
    st=localtime(&now);
    sprintf(timestamp,"%s, %d-%02d-%02d %02d:%02d:%02d"
            ,wday[st->tm_wday]
            ,st->tm_year+1900
            ,st->tm_mon+1
            ,st->tm_mday
            ,st->tm_hour+atoi(conf.daylight)
            ,st->tm_min
            ,st->tm_sec);

    if(st)
        free(st);

    log_start=1;
    message("%s - %s\n", timestamp, "Syslogd start up");
}

static void domark(int sig)
{
    if(log_start==0){
        router_start();
    }
    last_log[0]='\0';
    alarm(MarkInterval);
}

/* This must be a #define, since when DODEBUG and BUFFERS_GO_IN_BSS are
 * enabled, we otherwise get a "storage size isn't constant error. */
static int serveConnection (char* tmpbuf, int n_read)
{
    char *p = tmpbuf;

    while (p < tmpbuf + n_read) {

        int           pri = (LOG_USER | LOG_NOTICE);
        char          line[ MAXLINE + 1 ];
        unsigned char c;
        int find=0;

        char *q = line;

        while ( (c = *p) && q < &line[ sizeof (line) - 1 ]) {
            if (c == '<' && find==0) {
                /* Parse the magic priority number. */
                pri = 0;
                find= 1;
                while (isdigit (*(++p))) {
                    pri = 10 * pri + (*p - '0');
                }
                if (pri & ~(LOG_FACMASK | LOG_PRIMASK)){
                    pri = (LOG_USER | LOG_NOTICE);
                }
            } else if (c == '\n') {
                *q++ = ' ';
            } else if (iscntrl (c) && (c < 0177)) {
                *q++ = '^';
                *q++ = c ^ 0100;
            } else {
                *q++ = c;
            }
            p++;
        }
        *q = '\0';
        p++;
        /* Now log it */
        logMessage (pri, line);
    }
    return n_read;
}

#ifdef BB_FEATURE_REMOTE_LOG
static void init_RemoteLog (void)
{

    struct sockaddr_in remoteaddr;
    struct hostent *hostinfo;
    int len = sizeof(remoteaddr);
    int so_bc=1;

    memset(&remoteaddr, 0, len);

    remotefd = socket(AF_INET, SOCK_DGRAM, 0);

    if (remotefd < 0) {
        error_msg_and_die("cannot create socket");
    }

    remoteaddr.sin_family = AF_INET;

    /* Ron */
    /* allow boardcast */
    setsockopt(remotefd,SOL_SOCKET,SO_BROADCAST,&so_bc,sizeof(so_bc));
    hostinfo = gethostbyname(RemoteHost);
    remoteaddr.sin_addr = *(struct in_addr *) *hostinfo->h_addr_list;
    remoteaddr.sin_port = htons(RemotePort);

    /*
       Since we are using UDP sockets, connect just sets the default host and port
       for future operations
     */
    if ( 0 != (connect(remotefd, (struct sockaddr *) &remoteaddr, len))){
        error_msg_and_die("cannot connect to remote host %s:%d", RemoteHost, RemotePort);
    }
}
#endif

static void doSyslogd (void) __attribute__ ((noreturn));
static void doSyslogd (void)
{
    struct sockaddr_un sunx;
    socklen_t addrLength;
    struct timeval tv;
    int sock_fd;
    fd_set fds;
    time_t now;

    /* Set up signal handlers. */
    signal (SIGINT,  quit_signal);
    signal (SIGTERM, quit_signal);
    signal (SIGQUIT, quit_signal);
    signal (SIGHUP,  send_mail_signal);
    signal (SIGUSR1, clear_signal);
    signal (SIGUSR2, reload_signal);
    signal (SIGCHLD,  SIG_IGN);
#ifdef SIGCLD
    signal (SIGCLD,  SIG_IGN);
#endif
    signal (SIGALRM, domark);
    //wait ntp get correct time
    alarm (MarkInterval);

    /* Create the syslog file so realpath() can work. */
    if (realpath (_PATH_LOG, lfile) != NULL)
        unlink (lfile);

    memset (&sunx, 0, sizeof (sunx));
    sunx.sun_family = AF_UNIX;
    strncpy (sunx.sun_path, lfile, sizeof (sunx.sun_path));
    if ((sock_fd = socket (AF_UNIX, SOCK_DGRAM, 0)) < 0)
        perror_msg_and_die ("Couldn't get file descriptor for socket " _PATH_LOG);

    addrLength = sizeof (sunx.sun_family) + strlen (sunx.sun_path);
    if (bind(sock_fd, (struct sockaddr *) &sunx, addrLength) < 0)
        perror_msg_and_die ("Could not connect to socket " _PATH_LOG);

    if (chmod (lfile, 0666) < 0)
        perror_msg_and_die ("Could not set permission on " _PATH_LOG);


#ifdef BB_FEATURE_IPC_SYSLOG
    if (circular_logging == TRUE ){
        ipcsyslog_init();
    }
#endif

#ifdef BB_FEATURE_REMOTE_LOG
    if (doRemoteLog == TRUE){
        init_RemoteLog();
    }
#endif

    for (;;) {

        FD_ZERO (&fds);
        FD_SET (sock_fd, &fds);

        tv.tv_sec = 60;
        tv.tv_usec = 0;

        if (select (sock_fd+1, &fds, NULL, NULL, &tv) < 0) {
            if (errno == EINTR) {
                /* alarm may have happened. */
                continue;
            }
            perror_msg_and_die ("select error");
        }
        time(&now);
        if (FD_ISSET (sock_fd, &fds)) {
            int   i;
            RESERVE_BB_BUFFER(tmpbuf, BUFSIZ + 1);

            memset(tmpbuf, '\0', BUFSIZ+1);
            if ( (i = recv(sock_fd, tmpbuf, BUFSIZ, 0)) > 0) {
                serveConnection(tmpbuf, i);
            } else {
                perror_msg_and_die ("UNIX socket error");
            }
            RELEASE_BB_BUFFER (tmpbuf);
        }/* FD_ISSET() */
/**
        if((now - last_send_mail >= conf.email_interval) && buf->num) {
            send_mail_signal(0);
        }
**/
    } /* for main loop */
}

char *config_file_path;

int parse_config(char *conf_path);

static void clear_signal(int sig)
{
	if(!buf)
		return;
    buf->head=0;
    buf->tail=0;
}

static void reload_signal(int sig)
{
    parse_config(config_file_path);
}


/* Modify by Jeff -Feb.22.2005- */
int parse_config(char *conf_path)
{
    FILE *fp;
    char buf[2048];
//    char tmp[32];
//    int i;
#ifdef DEBUG
    printf("conf_path==%s\n",conf_path);
#endif
    if(conf_path==NULL)
        fp=fopen(__CONF_FILE,"r");
    else
        fp=fopen(conf_path,"r");

    if(fp==NULL)
        return FALSE;

    fread(buf,sizeof(buf),1,fp);
    fclose(fp);

    /* initial conf */
    bzero(&conf,sizeof(conf));
    memset(&conf.log_list,-1,sizeof(conf.log_list));
    /* initial conf */

    if(strstr(buf,"email_alert=1")) conf.mail_enable=1;

    /* if email is not enable ,we don't need to parser those config*/
    if(conf.mail_enable==1){
        if(strstr(buf,"mail_log_full=1")) conf.mail_log_full=1;
        strccpy2(conf.mail_server,buf,"smtp_mail_server=",'\n');
        strccpy2(conf.mail_receiver,buf,"email_alert_addr=",'\n');
        strccpy2(conf.mail_sender,buf,"email_return_addr=",'\n');
        strccpy2(conf.mail_subject,buf,"mail_subject=",'\n');
        strccpy2(conf.mail_subject_alert,buf,"mail_subject_alert=",'\n');
        strccpy2(conf.mail_keyword,buf,"mail_keyword=",'\n');
        strccpy2(conf.dos_thresholds,buf,"dos_thresholds=",'\n');
/**
        strccpy2(conf.mail_auth_enable,buf, "mail_auth_enable=",'\n');
        strccpy2(conf.mail_auth_user,buf,"mail_auth_user=",'\n');
        strccpy2(conf.mail_auth_pass,buf,"mail_auth_pass=",'\n');
        strccpy2(tmp, buf, "email_qlen=",'\n');
        conf.email_qlen = atoi(tmp);
        strccpy2(tmp, buf, "email_interval=",'\n');
        conf.email_interval = atoi(tmp)*60;
**/
    }


    strccpy2(conf.TZ,buf,"TZ=",'\n');

    /*setenv("TZ",conf.TZ,1);*/

    strccpy2(conf.daylight,buf,"daylight=",'\n');

        if(strstr(buf,"log_enable=1")) 
        {        
            conf.log_enable=1;

            strccpy2(conf.log_server,buf,"log_server=",'\n');

            if(strlen(conf.log_server) >= 7)
            {
    			if(RemoteHost!=NULL) free(RemoteHost);
    			RemoteHost = strdup(conf.log_server);
			    doRemoteLog = TRUE;
            }
            else
                doRemoteLog = FALSE;
        }
        else
            doRemoteLog = FALSE;

    strccpy2(conf.time_mode, buf, "time_mode=",'\n');

    strccpy2(conf.log_keyword,buf,"log_keyword=",'\n');
/**
    strccpy2(tmp, buf, "log_level=",'\n');
    for(i=0; i<(sizeof(conf.log_level)/sizeof(conf.log_level[0])); i++) {
        conf.log_level[i] = strchr(tmp, '0'+i)?1:0;
    }
**/
    return TRUE;
}

int syslogd_main(int argc, char **argv)
    //int main(int argc, char **argv)
{
    int opt;
#if ! defined(__uClinux__)
    int doFork = TRUE;
#endif

    char *p;


    /* do normal option parsing */
    while ((opt = getopt(argc, argv, "m:nO:R:f:LC")) > 0) {
        switch (opt) {
            case 'm':
                MarkInterval = atoi(optarg) * 60;
                break;
#if ! defined(__uClinux__)
            case 'n':
                doFork = FALSE;
                break;
#endif
            case 'O':
                logFilePath = strdup(optarg);
                break;
#ifdef BB_FEATURE_REMOTE_LOG
            case 'R':
                if(RemoteHost!=NULL) free(RemoteHost);
                RemoteHost = strdup(optarg);
                if ( (p = strchr(RemoteHost, ':'))){
                    RemotePort = atoi(p+1);
                    *p = '\0';
                }
                doRemoteLog = TRUE;
                break;
            case 'L':
                local_logging = TRUE;
                break;
#endif
#ifdef BB_FEATURE_IPC_SYSLOG
            case 'C':
                circular_logging = TRUE;
                break;
#endif
            case 'f':
                config_file_path=optarg;
                if(parse_config(optarg)==FALSE)
                    show_usage();
                break;

            default:
                show_usage();
        }
    }
#ifdef BB_FEATURE_REMOTE_LOG
    /* If they have not specified remote logging, then log locally */
    if (doRemoteLog == FALSE)
        local_logging = TRUE;
#endif

#ifdef SHOW_HOSTNAME
    /* Store away localhost's name before the fork */
    gethostname(LocalHostName, sizeof(LocalHostName));
    if ((p = strchr(LocalHostName, '.'))) {
        *p++ = '\0';
    }

    /* If Hostname is NULL or contain embeded space, Show LAN IP address instead*/
    if(!strlen(LocalHostName) || strchr(LocalHostName, ' ')) {
        int sockfd = -1;
        struct ifreq ifr;
        struct sockaddr_in *saddr;

        if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
            perror("user: socket creating failed");
        }
        else {
            strcpy(ifr.ifr_name, LOGIC_LAN_IFNAME);
            ifr.ifr_addr.sa_family = AF_INET;
            /* get ip address */
            if (ioctl(sockfd, SIOCGIFADDR, &ifr)==0) {
                saddr = (struct sockaddr_in *)&ifr.ifr_addr;
                strcpy(LocalHostName, (char *)inet_ntoa(saddr->sin_addr));
            }
        }
    }
#endif
    umask(0);

#if ! defined(__uClinux__)
    if (doFork == TRUE) {
        if (daemon(0, 1) < 0)
            perror_msg_and_die("daemon");
    }
#endif
    doSyslogd();

    return EXIT_SUCCESS;
}
#if 1
extern int klogd_main (int argc ,char **argv);

int main(int argc ,char **argv)
{
    int ret = 0;
    char *base = strrchr(argv[0], '/');

    if (strstr(base ? (base + 1) : argv[0], "syslogd"))
        ret = syslogd_main(argc,argv);
    else if (strstr(base ? (base + 1) : argv[0], "klogd"))
        ret = klogd_main(argc,argv);
    else
        show_usage();

    return ret;
}
#endif
/*
   Local Variables
   c-file-style: "linux"
   c-basic-offset: 4
   tab-width: 4
End:
 */
