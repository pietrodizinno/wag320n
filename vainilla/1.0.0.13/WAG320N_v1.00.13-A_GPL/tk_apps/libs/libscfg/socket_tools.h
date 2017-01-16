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


#ifndef _SOCKETTOOLS_
#define _SOCKETTOOLS_

/* path of socket log */
#ifndef SOCKET_LOG
#define SOCKET_LOG  "log"
#endif
#include "socket_header.h"
#include "nvram.h"
/* socket path */
#define SOCKET_PATH	"/tmp/scfgmgr_socket"

extern char *nvram_data;

int socket_connect(void);
/*
 * Read data from socket
 * @param	header	like packet's header
 * @param	data	save data in here
 * @param	infd	write data to here
 * @return	0 success -1 error
 */
int socket_read(scfgmgr_header **header,char **data,int infd);

/*
 * Write data to socket
 * @param	header	like packet's header
 * @param	data	data that you want write to socket
 * @param	infd	read data from here
 * @return	0 success -1 error
 */
int socket_write(scfgmgr_header *header,char *data,int infd);

/*
 * Save log
 * @param	msg	message 
 * @param	len	message length
 */
void socket_log(char *msg,int len);

/*
 * Communication with scfgmgr   
 * @param	shd	header for write 
 * @param	sdata	data for write
 * @param	rhd	header for read   
 * @param	rdata	data for read
 * @return	0 success -1 error
 */
int scfgmgr_connect(scfgmgr_header *shd,char *sdata,scfgmgr_header **rhd,char **rdata);

/*
 * Send command to scfgmgr
 * @param       cmd   command  
 * @return      0 success -1 error
 */
int scfgmgr_cmd(int cmd,char **rdata);

#define scfgmgr_commit() {nvram_commit();}
//{char *tmp; scfgmgr_cmd(SCFG_COMMIT,&tmp); }

/*
 * Get all configuration data from scfgmgr
 * @param       rdata   save data in this point
 * @return      0 success -1 error
 */
int scfgmgr_getall(char **rdata);	


int scfgmgr_get(char *data,char **rdata);	

/*
 * Save configuration data to scfgmgr
 * @param       data     data ,you want save
 * @param       value    value
 * @return      0 success -1 error
 */
int scfgmgr_set(char *name,char *value);
int scfgmgr_sendfile(char *data,int len);
int scfgmgr_console(char *data,char **rdata);

/*
 * Parse value form data
 * @param       name     parse this name's value
 * @return      value 
 */
char* value_parser(char *name);


#endif
