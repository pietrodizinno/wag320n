/* $Id: dma.h,v 1.1.1.1 2009-01-05 09:00:36 fred_fu Exp $ */

#ifndef _ASM_DMA_H
#define _ASM_DMA_H

#include <asm/arch/dma.h>

/* it's useless on the Etrax, but unfortunately needed by the new
   bootmem allocator (but this should do it for this) */

#define MAX_DMA_ADDRESS PAGE_OFFSET

/* From PCI */

#ifdef CONFIG_PCI
extern int isa_dma_bridge_buggy;
#else
#define isa_dma_bridge_buggy 	(0)
#endif

#endif /* _ASM_DMA_H */
