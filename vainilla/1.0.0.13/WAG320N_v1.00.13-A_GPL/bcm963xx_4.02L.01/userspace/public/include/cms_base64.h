/* ***** BEGIN LICENSE BLOCK *****
 *
 * Copyright (c) 2007 Broadcom Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * ***** END LICENSE BLOCK ***** */


#ifndef CMS_BASE64_H_
#define CMS_BASE64_H_

/** Encode a binary buffer in ASCII base64 encoding.
 * 
 * @param src    (IN)  Input binary buffer.
 * @param srclen (IN)  Length of input binary buffer.
 * @param dest   (OUT) This function will allocate a buffer and put the
 *             null terminated base64 ASCII string in it.  The caller is
 *             responsible for freeing this buffer.
 *
 * @return CmsRet enum.
 */ 
CmsRet cmsB64_encode(const unsigned char *src, UINT32 srclen, char **dest);


/** Decode a null terminated base64 ASCII string into a binary buffer.
 * 
 * @param base64Str (IN) Input base64 ASCII string.
 * @param binaryBuf (OUT) This function will allocate a buffer and put
 *                   the decoded binary data into the buffer.  The caller
 *                   is responsible for freeing the buffer.
 * @param binaryBufLen (OUT) The length of the binary buffer.
 * 
 * @return CmsRet enum.
 */
CmsRet cmsB64_decode(const char *base64Str, UINT8 **binaryBuf, UINT32 *binaryBufLen);

#endif /*CMS_BASE64_H_*/
