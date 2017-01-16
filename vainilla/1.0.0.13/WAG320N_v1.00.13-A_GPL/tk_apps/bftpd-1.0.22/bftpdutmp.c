#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "main.h"
#include "bftpdutmp.h"
#include "mypaths.h"
#include "logging.h"

FILE *bftpdutmp = NULL;
long bftpdutmp_offset = 0xFFFFFFFF;

void bftpdutmp_init()
{
    /* First we have to create the file if it doesn't exist */
    mkdir("/var/run/bftpd",0755);
    bftpdutmp = fopen(PATH_BFTPDUTMP, "a");
    if (bftpdutmp)
        fclose(bftpdutmp);
    /* Then we can open it for reading and writing */
    if (!(bftpdutmp = fopen(PATH_BFTPDUTMP, "r+"))) {
        control_printf(SL_FAILURE, "421-Could not open " PATH_BFTPDUTMP "\r\n"
                 "421 Server disabled for security reasons.");
        exit(1);
    }
    rewind(bftpdutmp);
}

void bftpdutmp_end()
{
    if (bftpdutmp) {
        if (bftpdutmp_offset != -1)
            bftpdutmp_log(0);
        fclose(bftpdutmp);
        bftpdutmp = NULL;
    }
}

void bftpdutmp_log(char type)
{
    struct bftpdutmp ut, tmp;
    long i;
    if (!bftpdutmp)
        return;
    memset((void *) &ut, 0, sizeof(ut));
    ut.bu_pid = getpid();
    if (type) {
        ut.bu_type = 1;
        strncpy(ut.bu_name, user, sizeof(ut.bu_name));
        strncpy(ut.bu_host, remotehostname, sizeof(ut.bu_host));
       /* Determine offset of first user marked dead */
        rewind(bftpdutmp);
        i = 0;
        while (fread((void *) &tmp, sizeof(tmp), 1, bftpdutmp)) {
            if (!tmp.bu_type)
                break;
            i++;
        }
        bftpdutmp_offset = i * sizeof(tmp);
    } else
        ut.bu_type = 0;
    time(&(ut.bu_time));
    fseek(bftpdutmp, bftpdutmp_offset, SEEK_SET);
    fwrite((void *) &ut, sizeof(ut), 1, bftpdutmp);
    fflush(bftpdutmp);
}

char bftpdutmp_pidexists(pid_t pid)
{
    struct bftpdutmp tmp;
    rewind(bftpdutmp);
    while (fread((void *) &tmp, sizeof(tmp), 1, bftpdutmp)) {
        if (tmp.bu_pid == pid)
            return 1;
    }
    return 0;
}

char bftpdutmp_userexists(char *username)
{
    struct bftpdutmp tmp;
    rewind(bftpdutmp);
    while (fread((void *) &tmp, sizeof(tmp), 1, bftpdutmp)) {
        if (!strcmp(tmp.bu_name, username))
            return 1;
    }
    return 0;
}
