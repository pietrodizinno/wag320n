#ifndef _PARSEDIR_H_
#define _PARSEDIR_H_
#include <stdio.h>

#include "libuser.h"
#include "libshare.h"
#include "libcase.h"

#ifndef MAX_FILE_LENGTH
#define MAX_FILE_LENGTH 1024
#endif

typedef struct usershare{
 char *share;
 char *dir;
 struct usershare *next;
}usershare;

void transfer(unsigned char *input,int flag);
void de_dotdot (char *file);
char *convertDir(char *dirname,usershare *share,char *current_share);
char *isShare(char *p1,usershare *p_usershare);
int getUserShare(char *usr, usershare **share);
void freeUserShare(usershare *share);
int getShareDir(char *sharename,char *dir);
int folderIsShare(char *folder,usershare *shares);
void getUpDir(char *dir);
int checkFolder(char *filename,usershare *shares,int *readonly, int *convert, int flag);
void from_utf8 (char *file_name);
void to_utf8 (char *file_name);


#endif
