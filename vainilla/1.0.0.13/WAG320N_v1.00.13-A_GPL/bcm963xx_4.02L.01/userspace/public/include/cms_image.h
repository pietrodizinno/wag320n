/***********************************************************************
 *
 *  Copyright (c) 2007  Broadcom Corporation
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


#ifndef __CMS_IMAGE_H__
#define __CMS_IMAGE_H__




/** Minimum size for image uploads.
 *  Config file uploads can be less than 2K.
 */
#define  CMS_IMAGE_MIN_LEN    2048


/*!\enum CmsImageFormat
 * \brief cms image formats that we recognize.
 */
typedef enum
{
    CMS_IMAGE_FORMAT_INVALID=0,   /**< invalid or unrecognized format */
    CMS_IMAGE_FORMAT_BROADCOM=1,  /**< broadcom image (with our header) */
    CMS_IMAGE_FORMAT_FLASH=2,     /**< raw flash image */
    CMS_IMAGE_FORMAT_XML_CFG=3    /**< CMS XML config file */
} CmsImageFormat;



/** Validate the given image and return the image format of the given image.
 * 
 * The image can be a broadcom image, whole image, or config file.
 * This function will also verify that the image will fit in flash.
 *
 * @param imagePtr (IN) image to be parsed.
 * @param imageLen (IN) Length of image in bytes.
 * @param msgHandle (IN) message handle from the caller.
 * 
 * @return CmsImageFormat enum.
 */
CmsImageFormat cmsImg_validateImage(const char *imagePtr, UINT32 imageLen, void *msgHandle);


/** Write the image to flash.
 *  The image can be a config file, a broadcom image, or flash image.
 *  This function will validate the image before writing to flash.
 *
 * @param imagePtr (IN) image to be written.  Surprisingly, for cfe+kernel+fs
 *                      flash writes, the image is modified, so we cannot
 *                      declare this parameter as const.
 * @param imageLen (IN) Length of image in bytes.
 * @param msgHandle (IN) message handle from the caller.
 * 
 * @return CmsRet enum.
 */
CmsRet cmsImg_writeImage(char *imagePtr, UINT32 imageLen, void *msgHandle);


/** Write a validated image in known format to flash.
 *  The image can be a config file, a broadcom image, or flash image.
 *
 * @param imagePtr (IN) image to be written.  Surprisingly, for cfe+kernel+fs
 *                      flash writes, the image is modified, so we cannot
 *                      declare this parameter as const.
 * @param imageLen (IN) Length of image in bytes.
 * @param format   (IN) CmsImageFormat of the image.
 * @param msgHandle (IN) message handle from the caller.
 * 
 * @return CmsRet enum.
 */
CmsRet cmsImg_writeValidatedImage(char *imagePtr, UINT32 imageLen, CmsImageFormat format, void *msgHandle);


/** Return the number of bytes available in the flash for storing an image.
 * 
 * Note this function does not return the size of the entire flash.
 * Rather, it returns the number of bytes available in
 * the flash for storing the image.
 * 
 * @return the number of bytes available in the flash for storing an image.
 */
UINT32 cmsImg_getImageFlashSize(void);


/** Return the number of bytes available in the flash for storing a config file.
 *
 * Note, if compressed config file is enabled, this function will return
 * a number that is larger than the actual number of bytes available in
 * the flash for storing the config file
 *
 * @return the number of bytes available for storing a config file.
 */
UINT32 cmsImg_getConfigFlashSize(void);


/** Return the actual number of bytes available in the flash for storing a config file.
 *
 * This returns the actual number of bytes available in the flash for
 * storing a config file.  External callers should not use this function.
 *
 * @return the number of bytes available for storing a config file.
 */
UINT32 cmsImg_getRealConfigFlashSize(void);


/** Return the size, in bytes, of the broadcom image tag header.
 * 
 * @return the number of bytes in the broadcom image tag header that is present
 *         at the beginning of broadcom images.
 */
UINT32 cmsImg_getBroadcomImageTagSize(void);


/** Safety margin to use when determining if an image will fit into the flash.
 *  Used by cmsImg_willFitInFlash().
 *
 * Not clear if this is really needed.
 */
#define  CMS_IMAGE_OVERHEAD          256

/** Return true if the image will fit in the flash.
 * 
 * Compares the given image length with the image flash size minus a
 * CMS_IMAGE_OVERHEAD margin.  Do we really need to have this margin?
 * Currently, only httpd uses this code.
 * 
 * @param imageLen (IN) Length of image in bytes.
 * 
 * @return True if the image will fit in the flash.
 */
UBOOL8 cmsImg_willFitInFlash(UINT32 imageLen);


/** Minimum length needed to detect if a buffer could be a config file.
 */
#define CMS_CONFIG_FILE_DETECTION_LENGTH 64

/** Check the first CMS_CONFIG_FILE_DETECTION_LENGTH bytes of the given
 *  buffer to see if this is a config file.
 * 
 * @param buf (IN) buffer containing the image to be analyzed.
 * 
 * @return TRUE if the image is likely to be a config file.
 */
UBOOL8 cmsImg_isConfigFileLikely(const char *buf);
 

/** Send a message to smd informing it that we are starting a big image download.
 * 
 * This function only needs to be called if we are doing a broadcom or
 * flash image download.  Not needed for config file downloads.
 * 
 * @param msgHandle (IN) the message handle to use to send the message.
 */
void cmsImg_sendLoadStartingMsg(void *msgHandle); 


/** Send a message to smd informing it that we are done with a big image download.
 * 
 * smd will now restart any applications that it killed while the image download
 * was in progress.  This only needs to be called if the image download failed
 * and we want the modem to continue working as normal.  If the image download
 * succeeded, then we will probably reboot the modem as the last step.  No
 * need to restart apps again.
 * 
 * @param msgHandle (IN) the message handle to use to send the message.
 */
void cmsImg_sendLoadDoneMsg(void *msgHandle); 


/** Send a reboot request msg to smd.
 * 
 * @param msgHandle (IN) the message handle to use to send the message.
 */
void cmsUtil_sendRequestRebootMsg(void *msgHandle);

#endif /*__CMS_IMAGE_H__*/

