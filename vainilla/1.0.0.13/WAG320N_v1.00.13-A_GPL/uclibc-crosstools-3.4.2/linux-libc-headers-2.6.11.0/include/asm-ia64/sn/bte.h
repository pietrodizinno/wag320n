/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 2000-2004 Silicon Graphics, Inc.  All Rights Reserved.
 */


#ifndef _ASM_IA64_SN_BTE_H
#define _ASM_IA64_SN_BTE_H

#include <linux/timer.h>
#include <linux/cache.h>
#include <asm/sn/types.h>


/* #define BTE_DEBUG */
/* #define BTE_DEBUG_VERBOSE */

#ifdef BTE_DEBUG
#  define BTE_PRINTK(x) printk x	/* Terse */
#  ifdef BTE_DEBUG_VERBOSE
#    define BTE_PRINTKV(x) printk x	/* Verbose */
#  else
#    define BTE_PRINTKV(x)
#  endif /* BTE_DEBUG_VERBOSE */
#else
#  define BTE_PRINTK(x)
#  define BTE_PRINTKV(x)
#endif	/* BTE_DEBUG */


/* BTE status register only supports 16 bits for length field */
#define BTE_LEN_BITS (16)
#define BTE_LEN_MASK ((1 << BTE_LEN_BITS) - 1)
#define BTE_MAX_XFER ((1 << BTE_LEN_BITS) * L1_CACHE_BYTES)


/* Define hardware */
#define BTES_PER_NODE 2


/* Define hardware modes */
#define BTE_NOTIFY (IBCT_NOTIFY)
#define BTE_NORMAL BTE_NOTIFY
#define BTE_ZERO_FILL (BTE_NOTIFY | IBCT_ZFIL_MODE)
/* Use a reserved bit to let the caller specify a wait for any BTE */
#define BTE_WACQUIRE (0x4000)
/* Use the BTE on the node with the destination memory */
#define BTE_USE_DEST (BTE_WACQUIRE << 1)
/* Use any available BTE interface on any node for the transfer */
#define BTE_USE_ANY (BTE_USE_DEST << 1)
/* macro to force the IBCT0 value valid */
#define BTE_VALID_MODE(x) ((x) & (IBCT_NOTIFY | IBCT_ZFIL_MODE))

#define BTE_ACTIVE		(IBLS_BUSY | IBLS_ERROR)
#define BTE_WORD_AVAILABLE	(IBLS_BUSY << 1)
#define BTE_WORD_BUSY		(~BTE_WORD_AVAILABLE)

/*
 * Some macros to simplify reading.
 * Start with macros to locate the BTE control registers.
 */
#define BTE_LNSTAT_LOAD(_bte)						\
			HUB_L(_bte->bte_base_addr)
#define BTE_LNSTAT_STORE(_bte, _x)					\
			HUB_S(_bte->bte_base_addr, (_x))
#define BTE_SRC_STORE(_bte, _x)						\
			HUB_S(_bte->bte_base_addr + (BTEOFF_SRC/8), (_x))
#define BTE_DEST_STORE(_bte, _x)					\
			HUB_S(_bte->bte_base_addr + (BTEOFF_DEST/8), (_x))
#define BTE_CTRL_STORE(_bte, _x)					\
			HUB_S(_bte->bte_base_addr + (BTEOFF_CTRL/8), (_x))
#define BTE_NOTIF_STORE(_bte, _x)					\
			HUB_S(_bte->bte_base_addr + (BTEOFF_NOTIFY/8), (_x))


/* Possible results from bte_copy and bte_unaligned_copy */
/* The following error codes map into the BTE hardware codes
 * IIO_ICRB_ECODE_* (in shubio.h). The hardware uses
 * an error code of 0 (IIO_ICRB_ECODE_DERR), but we want zero
 * to mean BTE_SUCCESS, so add one (BTEFAIL_OFFSET) to the error
 * codes to give the following error codes.
 */
#define BTEFAIL_OFFSET	1

typedef enum {
	BTE_SUCCESS,		/* 0 is success */
	BTEFAIL_DIR,		/* Directory error due to IIO access*/
	BTEFAIL_POISON,		/* poison error on IO access (write to poison page) */
	BTEFAIL_WERR,		/* Write error (ie WINV to a Read only line) */
	BTEFAIL_ACCESS,		/* access error (protection violation) */
	BTEFAIL_PWERR,		/* Partial Write Error */
	BTEFAIL_PRERR,		/* Partial Read Error */
	BTEFAIL_TOUT,		/* CRB Time out */
	BTEFAIL_XTERR,		/* Incoming xtalk pkt had error bit */
	BTEFAIL_NOTAVAIL,	/* BTE not available */
} bte_result_t;


/*
 * Structure defining a bte.  An instance of this
 * structure is created in the nodepda for each
 * bte on that node (as defined by BTES_PER_NODE)
 * This structure contains everything necessary
 * to work with a BTE.
 */
struct bteinfo_s {
	volatile __u64 notify ____cacheline_aligned;
	__u64 *bte_base_addr ____cacheline_aligned;
	spinlock_t spinlock;
	cnodeid_t bte_cnode;	/* cnode                            */
	int bte_error_count;	/* Number of errors encountered     */
	int bte_num;		/* 0 --> BTE0, 1 --> BTE1           */
	int cleanup_active;	/* Interface is locked for cleanup  */
	volatile bte_result_t bh_error;	/* error while processing   */
	volatile __u64 *most_rcnt_na;
};


/*
 * Function prototypes (functions defined in bte.c, used elsewhere)
 */
extern bte_result_t bte_copy(__u64, __u64, __u64, __u64, void *);
extern bte_result_t bte_unaligned_copy(__u64, __u64, __u64, __u64);
extern void bte_error_handler(unsigned long);

#define bte_zero(dest, len, mode, notification) \
	bte_copy(0, dest, len, ((mode) | BTE_ZERO_FILL), notification)

/*
 * The following is the prefered way of calling bte_unaligned_copy
 * If the copy is fully cache line aligned, then bte_copy is
 * used instead.  Since bte_copy is inlined, this saves a call
 * stack.  NOTE: bte_copy is called synchronously and does block
 * until the transfer is complete.  In order to get the asynch
 * version of bte_copy, you must perform this check yourself.
 */
#define BTE_UNALIGNED_COPY(src, dest, len, mode)                        \
	(((len & L1_CACHE_MASK) || (src & L1_CACHE_MASK) ||             \
	  (dest & L1_CACHE_MASK)) ?                                     \
	 bte_unaligned_copy(src, dest, len, mode) :              	\
	 bte_copy(src, dest, len, mode, NULL))


#endif	/* _ASM_IA64_SN_BTE_H */
