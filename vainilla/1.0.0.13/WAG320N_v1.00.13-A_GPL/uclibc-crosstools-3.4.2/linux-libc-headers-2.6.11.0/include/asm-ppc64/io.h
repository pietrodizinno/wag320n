#ifndef _PPC64_IO_H
#define _PPC64_IO_H

/* 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <asm/page.h>
#include <asm/byteorder.h>
#ifdef CONFIG_PPC_ISERIES 
#include <asm/iSeries/iSeries_io.h>
#endif  
#include <asm/memory.h>
#include <asm/delay.h>

#include <asm-generic/iomap.h>

#define __ide_mm_insw(p, a, c) _insw_ns((volatile __u16 *)(p), (a), (c))
#define __ide_mm_insl(p, a, c) _insl_ns((volatile __u32 *)(p), (a), (c))
#define __ide_mm_outsw(p, a, c) _outsw_ns((volatile __u16 *)(p), (a), (c))
#define __ide_mm_outsl(p, a, c) _outsl_ns((volatile __u32 *)(p), (a), (c))


#define SIO_CONFIG_RA	0x398
#define SIO_CONFIG_RD	0x399

#define SLOW_DOWN_IO

extern unsigned long isa_io_base;
extern unsigned long pci_io_base;
extern unsigned long io_page_mask;

#define MAX_ISA_PORT 0x10000

#define _IO_IS_VALID(port) ((port) >= MAX_ISA_PORT || (1 << (port>>PAGE_SHIFT)) \
			    & io_page_mask)

#ifdef CONFIG_PPC_ISERIES
/* __raw_* accessors aren't supported on iSeries */
#define __raw_readb(addr)	{ BUG(); 0; }
#define __raw_readw(addr)       { BUG(); 0; }
#define __raw_readl(addr)       { BUG(); 0; }
#define __raw_readq(addr)       { BUG(); 0; }
#define __raw_writeb(v, addr)   { BUG(); 0; }
#define __raw_writew(v, addr)   { BUG(); 0; }
#define __raw_writel(v, addr)   { BUG(); 0; }
#define __raw_writeq(v, addr)   { BUG(); 0; }
#define readb(addr)		iSeries_Read_Byte(addr)
#define readw(addr)		iSeries_Read_Word(addr)
#define readl(addr)		iSeries_Read_Long(addr)
#define writeb(data, addr)	iSeries_Write_Byte((data),(addr))
#define writew(data, addr)	iSeries_Write_Word((data),(addr))
#define writel(data, addr)	iSeries_Write_Long((data),(addr))
#define memset_io(a,b,c)	iSeries_memset_io((a),(b),(c))
#define memcpy_fromio(a,b,c)	iSeries_memcpy_fromio((a), (b), (c))
#define memcpy_toio(a,b,c)	iSeries_memcpy_toio((a), (b), (c))

#define inb(addr)		readb(((void *)(long)(addr)))
#define inw(addr)		readw(((void *)(long)(addr)))
#define inl(addr)		readl(((void *)(long)(addr)))
#define outb(data,addr)		writeb(data,((void *)(long)(addr)))
#define outw(data,addr)		writew(data,((void *)(long)(addr)))
#define outl(data,addr)		writel(data,((void *)(long)(addr)))
/*
 * The *_ns versions below don't do byte-swapping.
 * Neither do the standard versions now, these are just here
 * for older code.
 */
#define insw_ns(port, buf, ns)	_insw_ns((__u16 *)((port)+pci_io_base), (buf), (ns))
#define insl_ns(port, buf, nl)	_insl_ns((__u32 *)((port)+pci_io_base), (buf), (nl))
#else

static inline unsigned char __raw_readb(const volatile void *addr)
{
	return *(volatile unsigned char *)addr;
}
static inline unsigned short __raw_readw(const volatile void *addr)
{
	return *(volatile unsigned short *)addr;
}
static inline unsigned int __raw_readl(const volatile void *addr)
{
	return *(volatile unsigned int *)addr;
}
static inline unsigned long __raw_readq(const volatile void *addr)
{
	return *(volatile unsigned long *)addr;
}
static inline void __raw_writeb(unsigned char v, volatile void *addr)
{
	*(volatile unsigned char *)addr = v;
}
static inline void __raw_writew(unsigned short v, volatile void *addr)
{
	*(volatile unsigned short *)addr = v;
}
static inline void __raw_writel(unsigned int v, volatile void *addr)
{
	*(volatile unsigned int *)addr = v;
}
static inline void __raw_writeq(unsigned long v, volatile void *addr)
{
	*(volatile unsigned long *)addr = v;
}
#define readb(addr)		eeh_readb(addr)
#define readw(addr)		eeh_readw(addr)
#define readl(addr)		eeh_readl(addr)
#define readq(addr)		eeh_readq(addr)
#define writeb(data, addr)	eeh_writeb((data), (addr))
#define writew(data, addr)	eeh_writew((data), (addr))
#define writel(data, addr)	eeh_writel((data), (addr))
#define writeq(data, addr)	eeh_writeq((data), (addr))
#define memset_io(a,b,c)	eeh_memset_io((a),(b),(c))
#define memcpy_fromio(a,b,c)	eeh_memcpy_fromio((a),(b),(c))
#define memcpy_toio(a,b,c)	eeh_memcpy_toio((a),(b),(c))
#define inb(port)		eeh_inb((unsigned long)port)
#define outb(val, port)		eeh_outb(val, (unsigned long)port)
#define inw(port)		eeh_inw((unsigned long)port)
#define outw(val, port)		eeh_outw(val, (unsigned long)port)
#define inl(port)		eeh_inl((unsigned long)port)
#define outl(val, port)		eeh_outl(val, (unsigned long)port)

/*
 * The insw/outsw/insl/outsl macros don't do byte-swapping.
 * They are only used in practice for transferring buffers which
 * are arrays of bytes, and byte-swapping is not appropriate in
 * that case.  - paulus */
#define insb(port, buf, ns)	eeh_insb((port), (buf), (ns))
#define insw(port, buf, ns)	eeh_insw_ns((port), (buf), (ns))
#define insl(port, buf, nl)	eeh_insl_ns((port), (buf), (nl))
#define insw_ns(port, buf, ns)	eeh_insw_ns((port), (buf), (ns))
#define insl_ns(port, buf, nl)	eeh_insl_ns((port), (buf), (nl))

#define outsb(port, buf, ns)  _outsb((__u8 *)((port)+pci_io_base), (buf), (ns))
#define outsw(port, buf, ns)  _outsw_ns((__u16 *)((port)+pci_io_base), (buf), (ns))
#define outsl(port, buf, nl)  _outsl_ns((__u32 *)((port)+pci_io_base), (buf), (nl))

#endif

#define readb_relaxed(addr) readb(addr)
#define readw_relaxed(addr) readw(addr)
#define readl_relaxed(addr) readl(addr)
#define readq_relaxed(addr) readq(addr)

extern void _insb(volatile __u8 *port, void *buf, int ns);
extern void _outsb(volatile __u8 *port, const void *buf, int ns);
extern void _insw(volatile __u16 *port, void *buf, int ns);
extern void _outsw(volatile __u16 *port, const void *buf, int ns);
extern void _insl(volatile __u32 *port, void *buf, int nl);
extern void _outsl(volatile __u32 *port, const void *buf, int nl);
extern void _insw_ns(volatile __u16 *port, void *buf, int ns);
extern void _outsw_ns(volatile __u16 *port, const void *buf, int ns);
extern void _insl_ns(volatile __u32 *port, void *buf, int nl);
extern void _outsl_ns(volatile __u32 *port, const void *buf, int nl);

#define mmiowb()

/*
 * output pause versions need a delay at least for the
 * w83c105 ide controller in a p610.
 */
#define inb_p(port)             inb(port)
#define outb_p(val, port)       (udelay(1), outb((val), (port)))
#define inw_p(port)             inw(port)
#define outw_p(val, port)       (udelay(1), outw((val), (port)))
#define inl_p(port)             inl(port)
#define outl_p(val, port)       (udelay(1), outl((val), (port)))

/*
 * The *_ns versions below don't do byte-swapping.
 * Neither do the standard versions now, these are just here
 * for older code.
 */
#define outsw_ns(port, buf, ns)	_outsw_ns((__u16 *)((port)+pci_io_base), (buf), (ns))
#define outsl_ns(port, buf, nl)	_outsl_ns((__u32 *)((port)+pci_io_base), (buf), (nl))


#define IO_SPACE_LIMIT ~(0UL)

static inline void iosync(void)
{
        __asm__ __volatile__ ("sync" : : : "memory");
}

/* Enforce in-order execution of data I/O. 
 * No distinction between read/write on PPC; use eieio for all three.
 */
#define iobarrier_rw() eieio()
#define iobarrier_r()  eieio()
#define iobarrier_w()  eieio()

/*
 * 8, 16 and 32 bit, big and little endian I/O operations, with barrier.
 * These routines do not perform EEH-related I/O address translation,
 * and should not be used directly by device drivers.  Use inb/readb
 * instead.
 */
static inline int in_8(const volatile unsigned char *addr)
{
	int ret;

	__asm__ __volatile__("lbz%U1%X1 %0,%1; twi 0,%0,0; isync"
			     : "=r" (ret) : "m" (*addr));
	return ret;
}

static inline void out_8(volatile unsigned char *addr, int val)
{
	__asm__ __volatile__("stb%U0%X0 %1,%0; sync"
			     : "=m" (*addr) : "r" (val));
}

static inline int in_le16(const volatile unsigned short *addr)
{
	int ret;

	__asm__ __volatile__("lhbrx %0,0,%1; twi 0,%0,0; isync"
			     : "=r" (ret) : "r" (addr), "m" (*addr));
	return ret;
}

static inline int in_be16(const volatile unsigned short *addr)
{
	int ret;

	__asm__ __volatile__("lhz%U1%X1 %0,%1; twi 0,%0,0; isync"
			     : "=r" (ret) : "m" (*addr));
	return ret;
}

static inline void out_le16(volatile unsigned short *addr, int val)
{
	__asm__ __volatile__("sthbrx %1,0,%2; sync"
			     : "=m" (*addr) : "r" (val), "r" (addr));
}

static inline void out_be16(volatile unsigned short *addr, int val)
{
	__asm__ __volatile__("sth%U0%X0 %1,%0; sync"
			     : "=m" (*addr) : "r" (val));
}

static inline unsigned in_le32(const volatile unsigned *addr)
{
	unsigned ret;

	__asm__ __volatile__("lwbrx %0,0,%1; twi 0,%0,0; isync"
			     : "=r" (ret) : "r" (addr), "m" (*addr));
	return ret;
}

static inline unsigned in_be32(const volatile unsigned *addr)
{
	unsigned ret;

	__asm__ __volatile__("lwz%U1%X1 %0,%1; twi 0,%0,0; isync"
			     : "=r" (ret) : "m" (*addr));
	return ret;
}

static inline void out_le32(volatile unsigned *addr, int val)
{
	__asm__ __volatile__("stwbrx %1,0,%2; sync" : "=m" (*addr)
			     : "r" (val), "r" (addr));
}

static inline void out_be32(volatile unsigned *addr, int val)
{
	__asm__ __volatile__("stw%U0%X0 %1,%0; sync"
			     : "=m" (*addr) : "r" (val));
}

static inline unsigned long in_le64(const volatile unsigned long *addr)
{
	unsigned long tmp, ret;

	__asm__ __volatile__(
			     "ld %1,0(%2)\n"
			     "twi 0,%1,0\n"
			     "isync\n"
			     "rldimi %0,%1,5*8,1*8\n"
			     "rldimi %0,%1,3*8,2*8\n"
			     "rldimi %0,%1,1*8,3*8\n"
			     "rldimi %0,%1,7*8,4*8\n"
			     "rldicl %1,%1,32,0\n"
			     "rlwimi %0,%1,8,8,31\n"
			     "rlwimi %0,%1,24,16,23\n"
			     : "=r" (ret) , "=r" (tmp) : "b" (addr) , "m" (*addr));
	return ret;
}

static inline unsigned long in_be64(const volatile unsigned long *addr)
{
	unsigned long ret;

	__asm__ __volatile__("ld%U1%X1 %0,%1; twi 0,%0,0; isync"
			     : "=r" (ret) : "m" (*addr));
	return ret;
}

static inline void out_le64(volatile unsigned long *addr, unsigned long val)
{
	unsigned long tmp;

	__asm__ __volatile__(
			     "rldimi %0,%1,5*8,1*8\n"
			     "rldimi %0,%1,3*8,2*8\n"
			     "rldimi %0,%1,1*8,3*8\n"
			     "rldimi %0,%1,7*8,4*8\n"
			     "rldicl %1,%1,32,0\n"
			     "rlwimi %0,%1,8,8,31\n"
			     "rlwimi %0,%1,24,16,23\n"
			     "std %0,0(%3)\n"
			     "sync"
			     : "=&r" (tmp) , "=&r" (val) : "1" (val) , "b" (addr) , "m" (*addr));
}

static inline void out_be64(volatile unsigned long *addr, unsigned long val)
{
	__asm__ __volatile__("std%U0%X0 %1,%0; sync" : "=m" (*addr) : "r" (val));
}

#ifndef CONFIG_PPC_ISERIES 
#include <asm/eeh.h>
#endif

#endif /* _PPC64_IO_H */
