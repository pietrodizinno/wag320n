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

#ifndef _CONSTANT_H_
#define _CONSTANT_H_

//#define _USE_DEBUG_CONF_		1

#define LINE_SIZE                                   256

#define FSH_MaxUser                                 16
#define FSH_MaxShare                                24
#define FSH_MaxUserLen                              20
#define FSH_MaxShareLen                             20
#define FSH_MaxCommentLen                           48
#define FSH_MaxDirLen                               256
#define FSH_MaxPassLen                              15
#define FSH_ADMIN_UID                               501
#define FSH_ADMIN_GRP_UID							501
#define FSH_DefaultCodePage                         437

#define	HIDDEN_FILE_FOLDERS							"/.AppleDB/.AppleDouble/.AppleDesktop/.DS_Store/:2eDS_Store/Network Trash Folder/Temporary Items/$AttrDef/$BadClus/$Bitmap/$Boot/$Extend/$LogFile/$MFT/$MFTMirr/$Secure/$UpCase/$Volume/lost+found/System Volume Information/.swap/.twonkymedia.db/"

#define	FSH_ILLEGAL_SERVER_NAME_CHAR				"\"/\\[]:;|=.,+*?<> '`()#$%"
#define	FSH_ILLEGAL_SERVER_COMM_CHAR				"|:,\\'\""
#define FSH_ILLEGAL_ZONE_CHAR                       "\\]/\">[<.:;,|=+?`'"
#define FSH_ILLEGAL_WG_CHAR                         "\"/\\[]:;|=.,+*?<>"
#define FSH_SPECIAL_CHAR                            "\"$\\`"
#define	FSH_MEDIA_SERVICE_NAME_CHAR					"|:,\\'\""
#define	ILLEGAL_USER_CHAR							"\"/\\[]:;|=.,+*?<>'`"
#define	ILLEGAL_USER_COMM_CHAR						"|\\:\",\'"
#define	ILLEGAL_GROUP_CHAR							"\"/\\[]:;|=.,+*?<>'`$%@"
#define	ILLEGAL_SHARE_CHAR							"\"/\\[]:;|=.,+*?<>'`$%@"
#define	ILLEGAL_FOLDER_CHAR							"\\:*?\"<>|'`"
#define	ILLEGAL_SHARE_COMM_CHAR						"\\:,|\'\""


#define FSH_FLASH_SHARE							"FLASH"
#define FSH_HARD_SHARE							"HDD"
#define	FSH_SHARE_KEY_PREFIX					"storage_shared_folder_"
#define	FSH_USER_KEY_PREFIX						"storage_user_"

#define	FSH_ADMIN_NAME_KEY						"storage_admin_user"
#define	FSH_ADMIN_PASSWORD_KEY					"storage_admin_pass"

#define	FSH_STORAGE_LOCK						"/var/storage.lck"
#define	FSH_DEF_SHARE_CONF						"/etc/def_share.info"
#define FSH_USER_SMB_CONF						"/etc/samba/user_smb.conf"
#define	FSH_DEBUG_CONF							"/etc/storage.conf"
#define TMPFILE_MODE							"/tmp/tmpfile.XXXXXX"

#define FSH_SMB_CONF							"/etc/samba/smb.conf"
#define FSH_USER_SMB_DIR						"/etc/smb/"
#define FSH_SYS_PASSWD							"/etc/passwd"
#define FSH_SMB_PASSWD							"/etc/samba/smbpasswd"
#define FSH_INETD_CONF							"/etc/inetd.conf"
#define FSH_PASSWD_SALT							"sc"
                                                    
#define FSH_ADMINISTRATOR_USER					"admin"
#define FSH_ADMINISTRATOR_PASSWD				"admin"
#define FSH_DEFAULT_PASS						"zGf8Ikl50e3aPM"

#define	CMD_SMBPASSWD							"/usr/sbin/smbpasswd"

//Backup file flag
#define fPass                                       1                               //passwd
#define fGrp                                        2                               //group
#define fShare                                      4                               //share.info
#define fUser                                       8                               //usrgrp.info
#define fSmb                                        0x10                            //smb.conf
#define fSmbPass                                    0x20                            //smbpasswd
#define fWebPass                                    0x40                            //.htpasswd
#define fGroups                                     (fGrp|fUser|fShare|fSmb)
#define fUsers                                      (fPass|fGrp|fUser|fSmbPass)
#define fShares                                     (fSmb|fShare)

#define UID_Start                                   2000
#define GID_Start                                   2000

#endif /* _CONSTANT_H_ */

