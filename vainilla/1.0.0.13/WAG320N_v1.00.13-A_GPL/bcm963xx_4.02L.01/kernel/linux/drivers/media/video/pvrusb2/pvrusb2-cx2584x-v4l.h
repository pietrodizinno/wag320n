/*
 *
 *  $Id: pvrusb2-cx2584x-v4l.h,v 1.1.1.1 2009-01-05 09:00:49 fred_fu Exp $
 *
 *  Copyright (C) 2005 Mike Isely <isely@pobox.com>
 *  Copyright (C) 2004 Aurelien Alleaume <slts@free.fr>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef __PVRUSB2_CX2584X_V4L_H
#define __PVRUSB2_CX2584X_V4L_H

/*

   This module connects the pvrusb2 driver to the I2C chip level
   driver which handles combined device audio & video processing.
   This interface is used internally by the driver; higher level code
   should only interact through the interface provided by
   pvrusb2-hdw.h.

*/



#include "pvrusb2-i2c-core.h"

int pvr2_i2c_cx2584x_v4l_setup(struct pvr2_hdw *,struct pvr2_i2c_client *);


#endif /* __PVRUSB2_CX2584X_V4L_H */

/*
  Stuff for Emacs to see, in order to encourage consistent editing style:
  *** Local Variables: ***
  *** mode: c ***
  *** fill-column: 70 ***
  *** tab-width: 8 ***
  *** c-basic-offset: 8 ***
  *** End: ***
  */
