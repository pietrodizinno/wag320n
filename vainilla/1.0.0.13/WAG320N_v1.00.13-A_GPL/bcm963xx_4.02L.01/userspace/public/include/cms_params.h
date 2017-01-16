/***********************************************************************
 *
 *  Copyright (c) 2006-2007  Broadcom Corporation
 *  All Rights Reserved
 *
# 
# 
# This program may be linked with other software licensed under the GPL. 
# When this happens, this program may be reproduced and distributed under 
# the terms of the GPL. 
# 
# 
# 1. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS" 
#    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR 
#    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH 
#    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND 
#    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT, 
#    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR 
#    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE 
#    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR 
#    PERFORMANCE OF THE SOFTWARE. 
# 
# 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR 
#    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL, 
#    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY 
#    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN 
#    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; 
#    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE 
#    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS 
#    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY 
#    LIMITED REMEDY. 
#
 *
 ************************************************************************/

#ifndef __CMS_PARAMS_H__
#define __CMS_PARAMS_H__

/*!\file cms_params.h
 * \brief Header file containing customizable or board/hardware dependent
 *        parameters for the the CPE Management System (CMS).  Note that
 *        other customizable parameters are modified via make menuconfig.
 */


/** Config file version.
 *
 */
#define CMS_CONFIG_FILE_VERSION "1.0"


/** Number of spaces to indent each line in the config file.
 *
 */
#define CMS_CONFIG_FILE_INDENT 2


/** Address where the shared memory region is attached.
 *
 * Every process must attach to the shared memory at the same address
 * because the data structures inside the shared memory region contain
 * pointers to other areas in the shared memory.
 */
#define MDM_SHM_ATTACH_ADDR  0x58800000


/** Amount of shared memory to allocate.
 *
 * We need to error on the larger side because we don't want to run
 * out of shared memory.  If we don't touch the pages, the kernel will
 * not allocate them to us anyways.
 */
#define MDM_SHM_SIZE         (256 * 1024)


/** The "key" to use when requesting a semaphore from the Linux OS.
 *
 * This is used to implement low level MDM transation locks.
 * The only time this will need to be modified is when other code
 * is using the same key.
 */
#define MDM_LOCK_SEMAPHORE_KEY 0x5ed7


/** This is the Unix Domain Socket address for communications with smd used
 *  by the messaging library.
 *
 * Note two different addresses are defined, one for modem and one for DESKTOP_LINUX testing.
 *  It is highly unlikely that this needs to be changed.
 */
#ifdef DESKTOP_LINUX
#define SMD_MESSAGE_ADDR  "/var/tmp/smd_messaging_server_addr"
#else
#define SMD_MESSAGE_ADDR  "/var/smd_messaging_server_addr"
#endif


/** This is the number of fully connected connections that can be queued
 *  up at the SMD message server socket.
 *
 *  It is highly unlikely that this needs to be changed.
 */
#define SMD_MESSAGE_BACKLOG  3


/** Special hack for the smd dynamic launch service, when it launches a server app, the
 *  server app will find its server fd at this number.
 *
 * It is highly unlikely that this needs to be changed.
 */
#define CMS_DYNAMIC_LAUNCH_SERVER_FD  3



/** This is the port ftpd listens on.
 * 
 * Note two different ports are defined, one for modem and one for DESKTOP_LINUX testing.
 * It is highly unlikely that this needs to be changed.
 */
#ifdef DESKTOP_LINUX
#define FTPD_PORT       44421
#else
#define FTPD_PORT       21
#endif


/** This is the port tftpd listens on.
 * 
 * Note two different ports are defined, one for modem and one for DESKTOP_LINUX testing.
 * It is highly unlikely that this needs to be changed.
 */
#ifdef DESKTOP_LINUX
#define TFTPD_PORT      44469
#else
#define TFTPD_PORT      69
#endif


/** This is the port sshd listens on.
 * 
 * Note two different ports are defined, one for modem and one for DESKTOP_LINUX testing.
 * It is highly unlikely that this needs to be changed.
 */
#ifdef DESKTOP_LINUX
#define SSHD_PORT       44422
#else
#define SSHD_PORT       22
#endif


/** The amount of idle time, in seconds, before sshd exits.
 *
 * Make this relatively long because the user might be configuring something,
 * then gets confused and have to look up some manual.
 */
#define SSHD_EXIT_ON_IDLE_TIMEOUT  600


/** This is the port telnetd listens on.
 * 
 * Note two different ports are defined, one for modem and one for DESKTOP_LINUX testing.
 * It is highly unlikely that this needs to be changed.
 */
#ifdef DESKTOP_LINUX
#define TELNETD_PORT    44423
#else
#define TELNETD_PORT    23
#endif


/** The amount of idle time, in seconds, before telnetd exits.
 *
 * Make this relatively long because the user might be configuring something,
 * then gets confused and have to look up some manual.
 */
#define TELNETD_EXIT_ON_IDLE_TIMEOUT  600


/** This is the port httpd listens on.
 * 
 * Note two different ports are defined, one for modem and one for DESKTOP_LINUX testing.
 * It is highly unlikely that this needs to be changed.
 */
#ifdef DESKTOP_LINUX
#define HTTPD_PORT      44480
#else
#ifdef SUPPORT_HTTPD_SSL
#define HTTPD_PORT      443
#else
#define HTTPD_PORT      80
#endif
#endif


/** The amount of idle time, in seconds, before httpd exits.
 *
 * Make this relatively long because the user might be configuring something,
 * then gets confused and have to look up some manual.
 */
#define HTTPD_EXIT_ON_IDLE_TIMEOUT  600


/** The amount of idle time, in seconds, before consoled exits.
 *
 * Make this relatively long because the user might be configuring something,
 * then gets confused and have to look up some manual.
 */
#define CONSOLED_EXIT_ON_IDLE_TIMEOUT  600


/** This is the port snmpd listens on.
 * 
 * Note two different ports are defined, one for modem and one for DESKTOP_LINUX testing.
 * It is highly unlikely that this needs to be changed.
 */
#ifdef DESKTOP_LINUX
#define SNMPD_PORT      44161
#else
#define SNMPD_PORT      161
#endif

/** This is the port tr64c listens on.
* LGD_TODO: Due to the time limit, it still have one DESKTOP_LINUX version TR64C, 
* in the future will add it.
*/
#define TR64C_HTTP_CONN_PORT     5431


/** This is the port tr69c listens on for connection requests from the ACS.
 * 
 */
#define TR69C_CONN_REQ_PORT      30005


/** This is the path part of the URL for tr69c connection requests from the ACS.
 * 
 */
#define TR69C_CONN_REQ_PATH      "/"


/** The amount of idle time, in seconds, before tr69c exits.
 *
 * Don't want to make this too long because tr69c is holding the session
 * write lock during its entire run time.  So if tr69c has nothing to do,
 * it should exit so it can release the write lock and allow other apps to
 * change the config.
 */
#define TR69C_EXIT_ON_IDLE_TIMEOUT       30 


/** Maximum number of Layer 2 bridges supported.
 * 
 * If this value is changed, be sure to also modify the default value in
 * the data model.
 */
#define MAX_LAYER2_BRIDGES                16


/** Maximum depth of objects in the Data Model that we can support.
 *  If the data model has a greater actual depth than what is defined here,
 *  cmsMdm_init() will fail.
 */
#define MAX_MDM_INSTANCE_DEPTH    6


/** Maximum length of a parameter name in the Data Model that we can support.
 *  If the data model has a greater actual param name length than what is defined here,
 *  cmsMdm_init() will fail.
 */
#define MAX_MDM_PARAM_NAME_LENGTH   55


#endif  /* __CMS_PARAMS_H__ */
