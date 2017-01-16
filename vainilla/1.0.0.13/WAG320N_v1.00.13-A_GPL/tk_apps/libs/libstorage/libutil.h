/******************************************************************************************
 *
 *  Copyright - 2005 SerComm Corporation.
 *
 *      All Rights Reserved.
 *      SerComm Corporation reserves the right to make changes to this document without notice.
 *      SerComm Corporation makes no warranty, representation or guarantee regarding
 *      the suitability of its products for any particular purpose. SerComm Corporation assumes
 *      no liability arising out of the application or use of any product or circuit.
 *      SerComm Corporation specifically disclaims any and all liability,
 *      including without limitation consequential or incidental damages;
 *      neither does it convey any license under its patent rights, nor the rights of others.
 *
 *****************************************************************************************/
#ifndef _PROCESS_H_
#define _PROCESS_H_


#define MAX_ITEM_LEN	24
extern FILE *ifp;
typedef char Char128[128];
typedef char Char256[256];

typedef	struct multi_items {
	char key[MAX_ITEM_LEN];
	char *value;
	int flag;
} multi_items;

int SYSTEM(const char *format, ...);

/* batch access routines */
/* returns a temp file pointer */
FILE *PRO_uopen(char *fn);
/* for ctrl of u_to_sect() */
#define PRO_NWSEC   0
#define PRO_WRSEC   1

/* for ctrl of u_close() */
#define PRO_WRIRST     1
#define PRO_GIVEUP     2
#define PRO_GOWRIRST   3
int PRO_uclose(char *fn, int mode, int ctrl);
int PRO_2sect(char *sect, FILE *fp);

void StripEndingSpace(char *buf);
int ItemExist(char *pItem, char *pFile);
int DelKeyFromConf(char *key, char *fname);
int GetLineFromFile(char **lbuf,FILE *fp);
int DelKeyItem(char *name, char *fname, char tag);
void BackupConFiles(int flag);
void RestoreConFiles(int flag);
int readvkey(char *buf, int size, FILE * fh);
int FSH_GetStr(char *sect, char *key, char *val, int size, FILE * fp);
int FSH_GetStrNoCase(char *sect, char *key, char *val, int size, FILE* fp);
int FSH_GetStrLen(char *sect, char *key, FILE* fp);
int RenameKey(char *strOld, char *strNew, char *fname);
int RenameSect(char *strOld, char *strNew, char *fname);
void GetTmpFileName(char name[64]);
void Unlock(void);
int Lock(void);

/* single attribute access routines */
int PRO_GetInt(char *sect, char *key, int *val, FILE *fp);
int PRO_GetStr(char *sect, char *key, char *val, int size, FILE* fp);
int PRO_GetLong(char *sect, char *key, long *val, FILE *fp);
int PRO_SetInt(char *sect, char *key, int val, char *fname);
int PRO_SetStr(char *sect, char *key, char *val, char *fname);
int PRO_DelSect(char *sect,char *fname);
int PRO_SetMultiItemStr(char *sect, multi_items *val,int itemnum, char *fname);
int PRO_u2sect(char *sect, int ctrl);
void DBCS_SpecialChar(char *old, char *new,char *str);
int DelFile(char *path);
int DBCS_AnyDBCSInStr(char *str);
int IsHiddenFile(char *pFileName);
#endif
