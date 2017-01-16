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

#ifndef __USER_SMB_H__
#define __USER_SMB_H__

int AddOneUserSMB(char *pUser);
int DelOneUserSMB(char *pUser);
void RemoveShFromUserSMB(char *pUser, char *pShare);
void UpdateShInUserSMB(char *pUser, char *pOld, char *pNew);
void AddSh2UserSMB(char *pUser, char *pShare);
int RemoveOneShFromAllUserSMB(char *pShare);
int UpdateShInAllUserSMB(char *pOld, char *pNew);
int CreateAllUserSMB(void);
int UpdateOneUserSMBWithU(char *pOld,  char *pNew);
#endif
