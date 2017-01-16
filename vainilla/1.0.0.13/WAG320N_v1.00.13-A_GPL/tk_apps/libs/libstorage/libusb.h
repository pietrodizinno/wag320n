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

#ifndef	__DISK_TABLE_H__
#define	__DISK_TABLE_H__

#include <sys/types.h>

#include <errno.h>
#include <unistd.h>

#define	MAX_ATTACH_DEV			2
#define	MAX_MOUNT_VOL			10
#define	MAX_USB_PARTS			15
#define	FILE_SYS_STR_LEN		32
#define	SH_FOLDER_LEN			20
#define	MAX_PARTITIONS			4	
#define KILOBYTE 				1024
#define DSK_TYPE_LEN			30	//harddisk type name

#define	PROC_SCSI	"/proc/scsi/scsi"
#define	PROC_MOUNT	"/proc/mounts"

#define	USB_1_DEV_NAME	"/dev/sda"
#define	USB_2_DEV_NAME	"/dev/sdb"

#define	USB_1_INDEX		1
#define	USB_2_INDEX		2

#define	USER_ADMIN_UID	501

#define		PROC_FLASH_1		"/proc/flash_sda"
#define		PROC_FLASH_2		"/proc/flash_sdb"
#define		PROC_FLASH_3		"/proc/flash_sdc"
#define		PROC_FLASH_4		"/proc/flash_sdd"

#define		PROC_HDD_1		"/proc/hdd_sda"
#define		PROC_HDD_2		"/proc/hdd_sdb"
#define		PROC_HDD_3		"/proc/hdd_sdc"
#define		PROC_HDD_4		"/proc/hdd_sdd"


#define	USB_MOUNT_POINT	"/harddisk/usb_%d"
#define	HDD_PREFIX		"HDD"
#define	FLASH_PREFIX	"FLASH"
#define PARTITION_TMP_FILE	"/var/fdisk"
#define	SWAP_FILE_EXIST		"/var/usb_swap"
#define	SWAP_FILE_NAME	".swap"

#define	CMD_FDISK		"/usr/sbin/fdisk"
#define	CMD_MKDOSFS		"/usr/sbin/mkdosfs"
#define	CMD_MKNTFS		"/usr/sbin/mkntfs"
#define	CMD_SWAPOFF		"/sbin/swapoff"
#define	CMD_SWAPON		"/sbin/swapon"
#define	CMD_MKSWAP		"/sbin/mkswap"

#define	CMD_SMBD		"/usr/sbin/smbd"
#define	CMD_NMBD		"/usr/sbin/nmbd"
#define	CMD_INETD		"/usr/sbin/inetd"

struct dev_list{
	char hdd_flag[16];
	char flash_flag[16];
	char hdd_eject_flag[24];
	char flash_eject_flag[24];
	char media_flag[32];
	char dev_name[16];
	char share_name[12];
	char reformat_flag[24];
	int	mounts;
	int bad_dev;
};

typedef struct DISK_PHY_INFO {
	char	dsk_type[DSK_TYPE_LEN];	 // type of the hard disk.
	unsigned long	dsk_cylinders;   // cylinders of the hard disk.
	unsigned long	dsk_heads;    	 // heads of the hard disk
	unsigned long	dsk_sectors;  	 // sectors of the hard disk
	unsigned long long	dsk_size;      	 // hard disk size in bytes
} DISK_PHY_INFO;

typedef struct DISK_CAPACITY_INFO{
//	char 				disk_name[16];		//
	char				mode_type[DSK_TYPE_LEN];	 // type of the hard disk.
	unsigned long long	total_size;      	 // hard disk size in KB
	int					part_num[MAX_USB_PARTS];
	char				file_system[MAX_USB_PARTS][FILE_SYS_STR_LEN];
	char				shared_folder[MAX_USB_PARTS][SH_FOLDER_LEN+1];
	unsigned long long	part_total_size[MAX_USB_PARTS];
	unsigned long long	part_free_size[MAX_USB_PARTS];
}DISK_CAPACITY_INFO;

#define	FS_FAT32	0
#define	FS_NTFS		1

typedef struct DISK_FORMAT_INFO
{
	int part_enable[MAX_PARTITIONS];//0-Not Enabled, 1-Enabled
	unsigned long part_size[MAX_PARTITIONS];//MB
	int fs[MAX_PARTITIONS]; //0-FAT32, 1-NTFS
	int part_all_left[MAX_PARTITIONS]; //0-Not Enabled, 1-Enabled. If 1, then ignore left partitions
	int disk_num;
}DISK_FORMAT_INFO;

void USB_InitDev(struct dev_list *devices, int num);
int USB_MountDisk(struct dev_list *devices, int dev_no, int *total_mounts);
int USB_UnmountDisk(char *share_name, int dev_no);
int USB_PartitionAndFormat(DISK_FORMAT_INFO *part);
int USB_GetDiskPhyInfo(int usb_num, DISK_PHY_INFO *pDskInfo);
int USB_GetDiskStatus(int usb_num, DISK_CAPACITY_INFO *pStatus);
int USB_IsDevMounted(char *pDev);
int USB_IsStrangeDevMounted(char *pDev);
int USB_GetDiskType(char *pDev, char *type);//pDev: sda, sdb ...
void USB_StopServers(void);
void USB_StartServers(void);
void USB_ReloadServers(void);
int AnyDeviceReady(void);
void StopStorageFunction(void);
int CheckPartition(char *hdname);
void StripSpace (char *pStr);
void GetUSBSerialNum(int usb_no, char *pSN);
#endif

