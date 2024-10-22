/******************************************************************************************
 *
 *  Copyright - 2005 SerComm Corporation.
 *
 *	All Rights Reserved. 
 *	SerComm Corporation reserves the right to make changes to this document without notice.
 *	SerComm Corporation makes no warranty, representation or guarantee regarding 
 *	the suitability of its products for any particular purpose. SerComm Corporation assumes 
 *	no liability arising out of the application or use of any product or circuit.
 *	SerComm Corporation specifically disclaims any and all liability, 
 *	including without limitation consequential or incidental damages; 
 *	neither does it convey any license under its patent rights, nor the rights of others.
 *
 *****************************************************************************************/


#ifndef _NVRAM_
#define _NVRAM_
#include <stdio.h>
#include <string.h>
/*
 * 2003/02/27  		             			    
 * 		       				released by Ron     
 */

/* line terminator by 0x00 
 * data terminator by two 0x00
 * value separaed by 0x01
 */	

/* nvram path */
#ifndef TEST
#define NVRAM_PATH     "/dev/mtdblock3"          /* ex:  /dev/mtd/nvram */
#else
#define NVRAM_PATH     "/tmp/nvramall"            /* ex:  /dev/mtd/nvram */
#endif

#define NVRAM_TMP_PATH "/tmp/nvram"		  /* ex:  /tmp/nvram     */
#define NVRAM_DEFAULT  "/etc/default.en"             /* ex:  /etc/default   */
//#define NVRAM_JFFS2_PATH "/etc/nvram"

#define END_SYMBOL	    0x00		  	
#define DIVISION_SYMBOL	    0x01		  

/* NVRAM_HEADER MAGIC*/ 
#define NVRAM_MAGIC 		    0x004E4F52		 /* RON */

/* used 12bytes, 28bytes reserved */
#define NVRAM_HEADER_SIZE   40       		 
/* max size in flash*/
//#define NVRAM_SIZE          65535		  /* nvram size 64k bytes*/
#define NVRAM_SIZE              0x10000

/* each line max size*/
#define NVRAM_BUFF_SIZE           4096		 

/* errorno */
#define NVRAM_SUCCESS       	    0
#define NVRAM_FLASH_ERR           1 
#define NVRAM_MAGIC_ERR	    2
#define NVRAM_LEN_ERR	    3
#define NVRAM_CRC_ERR	    4
#define NVRAM_SHADOW_ERR	    5

/*
 * nvram header struct 		            
 * magic    = 0x004E4F52 (RON)             
 * len      = 0~65495                      
 * crc      = use crc-32                    
 * reserved = reserved 	                    
 */
 
typedef struct nvram_header_s{
	unsigned long magic;
	unsigned long len;
	unsigned long crc;
	unsigned long reserved;
	
}nvram_header_t;


/* Copy data from flash to NVRAM_TMP_PATH
 * @return	0 on success and errorno on failure     
 */
int nvram_load(void);


/*
 * Write data from NVRAM_TMP_PATH to flash   
 * @return	0 on success and errorno on failure     
 */
int nvram_commit(void);

/*
 * Get the value of an NVRAM variable
 * @param	name	name of variable to get
 * @return	value of variable or NULL if undefined
 */
#define nvram_get_def(name) nvram_get_fun(name,NVRAM_DEFAULT)
//#define nvram_safe_get(msg) (nvram_get(msg)?:"")
extern char* nvram_safe_get(const char *name);
extern char* nvram_get(const char *name);
char* nvram_get_fun(const char *name,char *path);
extern char*  nvram_getall(char *data,int bytes);
extern char* nvram_get_wps(const char *name);

/*
 * Match an NVRAM variable
 * @param	name	name of variable to match
 * @param	match	value to compare against value of variable
 * @return	TRUE if variable is defined and its value is string equal to match or FALSE otherwise
 */
static inline int nvram_match(char *name, char *match) {
	const char *value = nvram_get(name);
	return (value && !strcmp(value, match));
}

/*
 * IN_Match an NVRAM variable
 * @param	name	name of variable to match
 * @param	match	value to compare against value of variable
 * @return	TRUE if variable is defined and its value is not string equal to invmatch or FALSE otherwise
 */
static inline int nvram_invmatch(char *name, char *invmatch) {
	const char *value = nvram_get(name);
	return (value && strcmp(value, invmatch));
}

/*
 * Set the value of an NVRAM variable
 * @param	name	name of variable to set
 * @param	value	value of variable
 * @return	0 on success and errorno on failure
 * NOTE: use nvram_commit to commit this change to flash.
 */
extern int nvram_set(const char* name,const char* value);

#endif
