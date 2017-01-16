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

#ifndef _FTP_H_
#define _FTP_H_
#include "constant.h"

#define MAX_UPNP_NAME_LEN	32
#define MAX_AV_PATH_LEN		256
#define	FOLDER_NUM			4
#define	MAX_SERVER_NAME_LEN	20
#define	MAX_WORKGROUP_NAME_LEN	20
#define	ETC_SERVICE	"/etc/services"	

typedef struct upnp_av{
	int		upnp_av;
	char	upnp_name[MAX_UPNP_NAME_LEN+1];
	int		upnp_av_path_enable[FOLDER_NUM];
	char	upnp_av_path[FOLDER_NUM][MAX_AV_PATH_LEN+1];	
	int 	content_type[FOLDER_NUM];
	int		code_page;
	int		device_port;
	int		scan_interval;//-1-Continueous Scan
}UPNP_AV;

typedef struct ftp_conf{
	char server_name[MAX_SERVER_NAME_LEN+1];
	char workgroup_name[MAX_WORKGROUP_NAME_LEN+1];
    int ftp_server;
    int ftp_wan;
    int ftp_ssl;
    int ftp_change;
    int ftp_anon;
    int ftp_lang;
    int ftp_port;
}FTP_SERVER;

int ReadFTPConf(FTP_SERVER *pconf);
int SaveFTPConf(FTP_SERVER *pconf);
int ResetFTPServer(void);
int ResetUPnPAV(void);
void StartMediaServer(void);
void StopMediaServer(void);

int ReadUPnPAVConf(UPNP_AV *pConf);
int SaveUPnPAVConf(UPNP_AV *pConf);

int StripShareAndFolder(char *pSrc, char *pShare, char *pDir, int checkonly);


#endif
