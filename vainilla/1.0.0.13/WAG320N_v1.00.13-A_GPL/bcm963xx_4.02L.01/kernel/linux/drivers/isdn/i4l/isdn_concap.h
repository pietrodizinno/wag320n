/* $Id: isdn_concap.h,v 1.1.1.1 2009-01-05 09:00:45 fred_fu Exp $
 *
 * Linux ISDN subsystem, protocol encapsulation
 *
 * This software may be used and distributed according to the terms
 * of the GNU General Public License, incorporated herein by reference.
 *
 */

extern struct concap_device_ops isdn_concap_reliable_dl_dops;
extern struct concap_proto * isdn_concap_new( int );

