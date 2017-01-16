/*
 * A simple flash mapping code for BCM963xx board flash memory
 * It is simple because it only treats all the flash memory as ROM
 * It is used with chips/map_rom.c
 *
 *  Song Wang (songw@broadcom.com)
 */
/*
<:copyright-gpl
 Copyright 2004-2006 Broadcom Corp. All Rights Reserved.

 This program is free software; you can distribute it and/or modify it
 under the terms of the GNU General Public License (Version 2) as
 published by the Free Software Foundation.

 This program is distributed in the hope it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
:>
*/
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/config.h>

#include <board.h>
#include <bcmTag.h>
#include <bcm_map_part.h>	//kylin add
#if defined(CONFIG_SERCOMM_CODE)
#include <linux/mtd/partitions.h>
#else
#include <bcm_map_part.h>
#endif
#define  VERSION	"1.0"

extern PFILE_TAG kerSysImageTagGet(void);

static struct mtd_info *mymtd;

#if defined(CONFIG_SERCOMM_CODE)
#define PARTITION_NUM 4
static struct mtd_partition brcm_partition_info[PARTITION_NUM];
#define CFE_SIZE	(64 << 10)
#define FLASH_8M_SIZE	0x800000
#define FLASH_4M_SIZE	0x400000
#define NVRAM_SIZE	0x10000
#define TAG_SIZE	0x100
#define FS_KERNEL_SIZE_4M	0x3E0000
#define NVRAM_ADDR_4M	0x3F0000
#define LANG_ADDR_4M	0x3B0000
#define LANG_SIZE	0x40000
#endif

static map_word brcm_physmap_read16(struct map_info *map, unsigned long ofs)
{
	map_word val;
	
#if 0 /* We added voluntary preemption and locks to flash driver, so everything should go to flash driver such that locks are in effect */
    /* If the requested flash address is in a memory mapped range, use
     * __raw_readw.  Otherwise, use kerSysReadFromFlash.
     */
    if(((map->map_priv_1 & ~0xfff00000) + ofs + sizeof(short)) < map->map_priv_2)
	    val.x[0] = __raw_readw(map->map_priv_1 + ofs);
    else
#endif        
        kerSysReadFromFlash( &val.x[0], map->map_priv_1 + ofs, sizeof(short) );
	
	return val;
}

static void brcm_physmap_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
#if 0 /* We added voluntary preemption and locks to flash driver, so everything should go to flash driver such that locks are in effect */
    /* If the requested flash address is in a memory mapped range, use
     * memcpy_fromio.  Otherwise, use kerSysReadFromFlash.
     */
    if( ((map->map_priv_1 & ~0xfff00000) + from + len) < map->map_priv_2 )
	    memcpy_fromio(to, map->map_priv_1 + from, len);
    else
#endif        
        kerSysReadFromFlash( to, map->map_priv_1 + from, len );
}

static struct map_info brcm_physmap_map = {
	.name		= "Physically mapped flash",
	.bankwidth	= 2,
#if !defined(CONFIG_SERCOMM_CODE)	
	.read		= brcm_physmap_read16,
	.copy_from	= brcm_physmap_copy_from
#endif	
};

#if !defined(CONFIG_SERCOMM_CODE)
static int __init init_brcm_physmap(void)
{
	PFILE_TAG pTag = NULL;
	u_int32_t rootfs_addr, kernel_addr;

	printk("bcm963xx_mtd driver v%s\n", VERSION);

	/* Read the flash memory map from flash memory. */
	if (!(pTag = kerSysImageTagGet()))
	{
		printk("Failed to read image tag from flash\n");
		return -EIO;
	}

	rootfs_addr = (u_int32_t) simple_strtoul(pTag->rootfsAddress, NULL, 10) + BOOT_OFFSET;
	kernel_addr = (u_int32_t) simple_strtoul(pTag->kernelAddress, NULL, 10) + BOOT_OFFSET;
	
	brcm_physmap_map.size = kernel_addr - rootfs_addr;
	brcm_physmap_map.map_priv_1 = (unsigned long)rootfs_addr;

	/* Set map_priv_2 to the amount of flash memory that is memory mapped to
	 * the flash base address.  On the BCM6338, serial flash parts are only
	 * memory mapped up to 1MB even though the flash part may be bigger.
	 */
	brcm_physmap_map.map_priv_2 =(unsigned long)kerSysMemoryMappedFlashSizeGet();

	if (!brcm_physmap_map.map_priv_1) {
		printk("Wrong rootfs starting address\n");
		return -EIO;
	}
	
	if (brcm_physmap_map.size <= 0) {
		printk("Wrong rootfs size\n");
		return -EIO;
	}	
	
	mymtd = do_map_probe("map_rom", &brcm_physmap_map);
	if (mymtd) {
		mymtd->owner = THIS_MODULE;
		add_mtd_device(mymtd);

		return 0;
	}
	return -ENXIO;
}
#else
static int __init init_brcm_physmap(void)
{
	PFILE_TAG pTag = NULL;
	u_int32_t rootfs_addr, kernel_addr, fs_len, cfe_len;

	printk("bcm963xx_mtd driver v%s\n", VERSION);

	/* Read the flash memory map from flash memory. */
	if (!(pTag = kerSysImageTagGet()))
	{
		printk("Failed to read image tag from flash\n");
		return -EIO;
	}

	rootfs_addr = (u_int32_t) simple_strtoul(pTag->rootfsAddress, NULL, 10) + BOOT_OFFSET;
	kernel_addr = (u_int32_t) simple_strtoul(pTag->kernelAddress, NULL, 10) + BOOT_OFFSET;
	
	printk("bcm963xx_mtd  kernel_addr=%x rootfs_addr=%x\n",kernel_addr,rootfs_addr);
	
	//brcm_physmap_map.size = kernel_addr - rootfs_addr;
	//brcm_physmap_map.map_priv_1 = (unsigned long)rootfs_addr;
	brcm_physmap_map.size = FLASH_8M_SIZE;
	brcm_physmap_map.map_priv_1 = (unsigned long)FLASH_BASE;
	brcm_physmap_map.virt = (unsigned long)FLASH_BASE;
	fs_len = kernel_addr - rootfs_addr;
	cfe_len = CFE_SIZE;

	printk("brcm_physmap_map.map_priv_1=%x\n",brcm_physmap_map.map_priv_1);
	
	/* Set map_priv_2 to the amount of flash memory that is memory mapped to
	 * the flash base address.  On the BCM6338, serial flash parts are only
	 * memory mapped up to 1MB even though the flash part may be bigger.
	 */
	//brcm_physmap_map.map_priv_2 =(unsigned long)kerSysMemoryMappedFlashSizeGet();

	if (!brcm_physmap_map.map_priv_1) {
		printk("Wrong rootfs starting address\n");
		return -EIO;
	}
	
	if (brcm_physmap_map.size <= 0) {
		printk("Wrong rootfs size\n");
		return -EIO;
	}	
	printk("do_map_probe\n");
	
	mymtd = do_map_probe("cfi_probe", &brcm_physmap_map);
	printk("do_map_probe finish\n");
	if (!mymtd)
		return -EIO;

	printk("mymtd = %x\n", mymtd);
	mymtd->owner                      = THIS_MODULE;
	/* Solomon file system */
	
	/* Solomon bootloader */
	brcm_partition_info[0].name       = "bootloader";
	brcm_partition_info[0].offset     = 0;
	brcm_partition_info[0].size       = cfe_len;
	brcm_partition_info[0].mask_flags = 0;
	
	brcm_partition_info[1].name       = "fs";
	brcm_partition_info[1].offset     = cfe_len + TAG_SIZE;
	brcm_partition_info[1].size       = fs_len;
	brcm_partition_info[1].mask_flags = 0;
	
	/* Solomon tag + file system + kernel */
	brcm_partition_info[2].name       = "tag+fs+kernel";
	brcm_partition_info[2].offset     = cfe_len;
	brcm_partition_info[2].size       = FLASH_8M_SIZE-cfe_len-NVRAM_SIZE; 
	brcm_partition_info[2].mask_flags = 0;	
        
	/* Solomon nvram */
	brcm_partition_info[3].name       = "nvram";
	brcm_partition_info[3].offset     = FLASH_8M_SIZE-NVRAM_SIZE;
	brcm_partition_info[3].size       = NVRAM_SIZE;
	brcm_partition_info[3].mask_flags = 0;

// 	/* Leon tag1+fs1 */
//	brcm_partition_info[4].name       = "tag1+fs1";
//	brcm_partition_info[4].offset     = cfe_len;
//	brcm_partition_info[4].size       = FS_KERNEL_SIZE_4M;
//	brcm_partition_info[4].mask_flags = 0;
//	
//	/* Leon fs1 */
//	brcm_partition_info[5].name       = "fs1";
//	brcm_partition_info[5].offset     = cfe_len + TAG_SIZE;
//	brcm_partition_info[5].size       = simple_strtol(FLASH_BASE + cfe_len,NULL,10);
//	brcm_partition_info[5].mask_flags = 0;
//    
//    	/* Kenneth language package */
//	brcm_partition_info[6].name       = "lang";
//	brcm_partition_info[6].offset     = FLASH_4M_SIZE + LANG_ADDR_4M;
//	brcm_partition_info[6].size       = LANG_SIZE;
//	brcm_partition_info[6].mask_flags = 0;

	add_mtd_partitions(mymtd, brcm_partition_info, PARTITION_NUM);
	return 0;
}
#endif
static void __exit cleanup_brcm_physmap(void)
{
	if (mymtd) {
		del_mtd_device(mymtd);
		map_destroy(mymtd);
	}
	if (brcm_physmap_map.map_priv_1) {
		brcm_physmap_map.map_priv_1 = 0;
	}
}

module_init(init_brcm_physmap);
module_exit(cleanup_brcm_physmap);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Song Wang songw@broadcom.com");
MODULE_DESCRIPTION("Configurable MTD map driver for read-only root file system");
