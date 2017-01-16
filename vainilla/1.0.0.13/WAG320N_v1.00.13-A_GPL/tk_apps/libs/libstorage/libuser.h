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
#ifndef _USER_H_
#define _USER_H_
#include "constant.h"

typedef struct u_list {
    char name[FSH_MaxUserLen + 1];	// name of user
    int nAccess;		// Users that can access the share
    struct u_list *pNext;
} u_list;

typedef struct user_info {
    char name[FSH_MaxUserLen + 1];	// name of  user
    char password[FSH_MaxPassLen + 1];	// password of user
    int nUid;			// id of the user
} u_info;


typedef struct user_list {
    u_info u_info;				// user info (name, comment, quota, passwd)
    struct user_list *pNext;	// info of next user
} user_list;


int AddUser(u_info * pUser);
int DelUser(char *user);
int UpdateUser(char *pName, u_info * pUser);
int AddUserToShare(char *pUser, char *pShare, int nReadonly);
int DelUserFromShare(char *pUser, char *pShare);
int GetUserNameList(u_list ** ppList);
void FreeUserNameList(u_list * pList);
void FreeUserList(user_list * pList);
int IsUserExist(char *name);
int GetNumOfUsers(void);
int GetAllUser(user_list ** ppList);
int GetUserInfo(u_info * pInfo);
int RestorePass(char *pUser);
int CreateUserConf(void);
void DelUserSessions(char *szUser);
#endif  
