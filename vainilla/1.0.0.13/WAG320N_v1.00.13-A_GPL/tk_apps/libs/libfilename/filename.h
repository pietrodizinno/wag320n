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

#ifndef __FILENAME_H__
#define	__FILENAME_H__
typedef int BOOL;

BOOL strequal(const char *s1, const char *s2);
void strlower_m(char *s);
void strupper_m(char *s);
BOOL strequal(const char *s1, const char *s2);
char *strrchr_m(const char *s, char c);
char *strchr_m(const char *src, char c);
BOOL strequal(const char *s1, const char *s2);
#endif
