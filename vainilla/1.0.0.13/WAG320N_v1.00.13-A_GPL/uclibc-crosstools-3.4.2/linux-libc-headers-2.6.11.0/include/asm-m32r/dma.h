#ifndef _ASM_M32R_DMA_H
#define _ASM_M32R_DMA_H

/* $Id: dma.h 5224 2004-10-21 15:15:40Z mmazur $ */

#include <asm/io.h>

/*
 * The maximum address that we can perform a DMA transfer
 * to on this platform
 */
#define MAX_DMA_ADDRESS      (PAGE_OFFSET+0x20000000)

#endif /* _ASM_M32R_DMA_H */
