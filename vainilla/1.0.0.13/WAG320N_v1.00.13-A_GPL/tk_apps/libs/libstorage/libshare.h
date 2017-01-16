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

#ifndef __SHARE_H__
#define __SHARE_H__

#include "constant.h"
#include "libuser.h"

typedef struct sh_info{
	char	name[FSH_MaxShareLen+1];		// name of share
	char	comment[FSH_MaxCommentLen+1];	// comment of share
	char 	dir[FSH_MaxDirLen+1];	// folder of share
	int		hard_index;
}sh_info;

typedef struct sh_list{
	sh_info		sh;
	u_list		*pUser;				// Users that can access the share
	struct 		sh_list *pNext;		// pNext share
}sh_list;

typedef struct shname_list{
    char		name[FSH_MaxShareLen+1];		// name of share
    struct		shname_list *pNext;	    
}shname_list;

// share routins
int AddShare(sh_info * sh);
int DelShare(char *name);
int DeldefSharesInNVRAM(int dev_no);
int ShareExist(char *name);
int UpdateShare(char *name, sh_info * sh);
int GetAllShare(sh_list ** ppList);
void FreeShareList(sh_list * pList);
int GetShareInfo(sh_info * pSh);
int GetShareNameList(shname_list ** ppList);
void FreeShareNameList(shname_list * pList);
int GetNumOfShares(void);
int AddDefShares(char *pSharePrefix, int dev_no);
int DelDefShares(char *share_name, int dev_no, int mounts);
int CreateShareConf(void);
int IsFolderShared(char *pDir);
int ReadRealShareNameByStr(char *pShare, char *pRealName);
#endif
