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


#ifndef __CMS_DLIST_H__
#define __CMS_DLIST_H__

#include "cms.h"


typedef struct dlist_node {
	struct dlist_node *next;
	struct dlist_node *prev;
} DlistNode;


/** Initialize a field in a structure that is used as the head of a dlist */
#define DLIST_HEAD_IN_STRUCT_INIT(field) do {\
      (field).next = &(field);               \
      (field).prev = &(field);               \
   } while (0);

/** Initialize a standalone variable that is the head of a dlist */
#define DLIST_HEAD_INIT(name) { &(name), &(name) }

/** Declare a standalone variable that is the head of the dlist */
#define DLIST_HEAD(name) \
	struct dlist_node name = DLIST_HEAD_INIT(name)


/** Return true if the dlist is empty.
 *
 * @param head pointer to the head of the dlist.
 */
static inline int dlist_empty(const struct dlist_node *head)
{
	return ((head->next == head) && (head->prev == head));
}


/** add a new entry after an existing list element
 *
 * @param new       new entry to be added
 * @param existing  list element to add the new entry after.  This could
 *                  be the list head or it can be any element in the dlist.
 *
 */
static inline void dlist_append(struct dlist_node *new, struct dlist_node *existing)
{
   existing->next->prev = new;

   new->next = existing->next;
   new->prev = existing;

   existing->next = new;
}


/** add a new entry in front of an existing list element
 *
 * @param new       new entry to be added
 * @param existing  list element to add the new entry in front of.  This could
 *                  be the list head or it can be any element in the dlist.
 *
 */
static inline void dlist_prepend(struct dlist_node *new, struct dlist_node *existing)
{
   existing->prev->next = new;

   new->next = existing;
   new->prev = existing->prev;

   existing->prev = new;
}


/** Unlink the specified entry from the list.
 *  This function does not free the entry.  Caller is responsible for
 *  doing that if applicable.
 *
 * @param entry existing dlist entry to be unlinked from the dlist.
 */
static inline void dlist_unlink(struct dlist_node *entry)
{
   entry->next->prev = entry->prev;
   entry->prev->next = entry->next;

	entry->next = 0;
	entry->prev = 0;
}

/** Return byte offset of the specified member.
 *
 * This is defined in stddef.h for MIPS, but not defined
 * on LINUX desktop systems.  Play it safe and just define
 * it here for all build types.
 */
#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)


/** cast a member of a structure out to the containing structure
 *
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})


#define dlist_entry(ptr, type, member) \
	container_of(ptr, type, member)


/** Create a for loop over all entries in the dlist.
 *
 * @param pos A variable that is the type of the structure which
 *            contains the DlistNode.
 * @param head Pointer to the head of the dlist.
 * @param member The field name of the DlistNode field in the
 *               containing structure.
 *
 */
#define dlist_for_each_entry(pos, head, member)				\
	for (pos = dlist_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = dlist_entry(pos->member.next, typeof(*pos), member))



#endif  /*__CMS_DLIST_H__ */
