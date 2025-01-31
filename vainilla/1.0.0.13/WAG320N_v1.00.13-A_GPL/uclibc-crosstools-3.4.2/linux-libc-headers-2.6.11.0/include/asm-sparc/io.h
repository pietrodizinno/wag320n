/*
 */
#ifndef __SPARC_IO_H
#define __SPARC_IO_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/ioport.h>  /* struct resource */

#include <asm/page.h>      /* IO address mapping routines need this */
#include <asm/system.h>

#define page_to_phys(page)	(((page) - mem_map) << PAGE_SHIFT)

static inline __u32 flip_dword (__u32 l)
{
	return ((l&0xff)<<24) | (((l>>8)&0xff)<<16) | (((l>>16)&0xff)<<8)| ((l>>24)&0xff);
}

static inline __u16 flip_word (__u16 w)
{
	return ((w&0xff) << 8) | ((w>>8)&0xff);
}

#define mmiowb()

/*
 * Memory mapped I/O to PCI
 */

static inline __u8 __raw_readb(const volatile void *addr)
{
	return *volatile __u8 *)addr;
}

static inline __u16 __raw_readw(const volatile void *addr)
{
	return *volatile __u16 *)addr;
}

static inline __u32 __raw_readl(const volatile void *addr)
{
	return *volatile __u32 *)addr;
}

static inline void __raw_writeb(__u8 b, volatile void *addr)
{
	*volatile __u8 *)addr = b;
}

static inline void __raw_writew(__u16 w, volatile void *addr)
{
	* volatile __u16 *)addr = w;
}

static inline void __raw_writel(__u32 l, volatile void *addr)
{
	*(volatile __u32 *)addr = l;
}

static inline __u8 __readb(const volatile void *addr)
{
	return *(volatile __u8 *)addr;
}

static inline __u16 __readw(const volatile void *addr)
{
	return flip_word(*(volatile __u16 *)addr);
}

static inline __u32 __readl(const volatile void *addr)
{
	return flip_dword(*(volatile __u32 *)addr);
}

static inline void __writeb(__u8 b, volatile void *addr)
{
	*(volatile __u8 *)addr = b;
}

static inline void __writew(__u16 w, volatile void *addr)
{
	*(volatile __u16 *)addr = flip_word(w);
}

static inline void __writel(__u32 l, volatile void *addr)
{
	*(volatile __u32 *)addr = flip_dword(l);
}

#define readb(__addr)		__readb(__addr)
#define readw(__addr)		__readw(__addr)
#define readl(__addr)		__readl(__addr)
#define readb_relaxed(__addr)	readb(__addr)
#define readw_relaxed(__addr)	readw(__addr)
#define readl_relaxed(__addr)	readl(__addr)

#define writeb(__b, __addr)	__writeb((__b),(__addr))
#define writew(__w, __addr)	__writew((__w),(__addr))
#define writel(__l, __addr)	__writel((__l),(__addr))

/*
 * I/O space operations
 *
 * Arrangement on a Sun is somewhat complicated.
 *
 * First of all, we want to use standard Linux drivers
 * for keyboard, PC serial, etc. These drivers think
 * they access I/O space and use inb/outb.
 * On the other hand, EBus bridge accepts PCI *memory*
 * cycles and converts them into ISA *I/O* cycles.
 * Ergo, we want inb & outb to generate PCI memory cycles.
 *
 * If we want to issue PCI *I/O* cycles, we do this
 * with a low 64K fixed window in PCIC. This window gets
 * mapped somewhere into virtual kernel space and we
 * can use inb/outb again.
 */
#define inb_local(__addr)	__readb((void *)(unsigned long)(__addr))
#define inb(__addr)		__readb((void *)(unsigned long)(__addr))
#define inw(__addr)		__readw((void *)(unsigned long)(__addr))
#define inl(__addr)		__readl((void *)(unsigned long)(__addr))

#define outb_local(__b, __addr)	__writeb(__b, (void *)(unsigned long)(__addr))
#define outb(__b, __addr)	__writeb(__b, (void *)(unsigned long)(__addr))
#define outw(__w, __addr)	__writew(__w, (void *)(unsigned long)(__addr))
#define outl(__l, __addr)	__writel(__l, (void *)(unsigned long)(__addr))

#define inb_p(__addr)		inb(__addr)
#define outb_p(__b, __addr)	outb(__b, __addr)
#define inw_p(__addr)		inw(__addr)
#define outw_p(__w, __addr)	outw(__w, __addr)
#define inl_p(__addr)		inl(__addr)
#define outl_p(__l, __addr)	outl(__l, __addr)

void outsb(unsigned long addr, const void *src, unsigned long cnt);
void outsw(unsigned long addr, const void *src, unsigned long cnt);
void outsl(unsigned long addr, const void *src, unsigned long cnt);
void insb(unsigned long addr, void *dst, unsigned long count);
void insw(unsigned long addr, void *dst, unsigned long count);
void insl(unsigned long addr, void *dst, unsigned long count);

#define IO_SPACE_LIMIT 0xffffffff

/*
 * SBus accessors.
 *
 * SBus has only one, memory mapped, I/O space.
 * We do not need to flip bytes for SBus of course.
 */
static inline __u8 _sbus_readb(const volatile void *addr)
{
	return *(volatile __u8 *)addr;
}

static inline __u16 _sbus_readw(const volatile void *addr)
{
	return *(volatile __u16 *)addr;
}

static inline __u32 _sbus_readl(const volatile void *addr)
{
	return *(volatile __u32 *)addr;
}

static inline void _sbus_writeb(__u8 b, volatile void *addr)
{
	*(volatile __u8 *)addr = b;
}

static inline void _sbus_writew(__u16 w, volatile void *addr)
{
	*(volatile __u16 *)addr = w;
}

static inline void _sbus_writel(__u32 l, volatile void *addr)
{
	*(volatile __u32 *)addr = l;
}

/*
 * The only reason for #define's is to hide casts to unsigned long.
 */
#define sbus_readb(__addr)		_sbus_readb(__addr)
#define sbus_readw(__addr)		_sbus_readw(__addr)
#define sbus_readl(__addr)		_sbus_readl(__addr)
#define sbus_writeb(__b, __addr)	_sbus_writeb(__b, __addr)
#define sbus_writew(__w, __addr)	_sbus_writew(__w, __addr)
#define sbus_writel(__l, __addr)	_sbus_writel(__l, __addr)

static inline void sbus_memset_io(volatile void *__dst, int c, __kernel_size_t n)
{
	while(n--) {
		sbus_writeb(c, __dst);
		__dst++;
	}
}

static inline void
_memset_io(volatile void *dst, int c, __kernel_size_t n)
{
	volatile void *d = dst;

	while (n--) {
		writeb(c, d);
		d++;
	}
}

#define memset_io(d,c,sz)	_memset_io(d,c,sz)

static inline void
_memcpy_fromio(void *dst, const volatile void *src, __kernel_size_t n)
{
	char *d = dst;

	while (n--) {
		char tmp = readb(src);
		*d++ = tmp;
		src++;
	}
}

#define memcpy_fromio(d,s,sz)	_memcpy_fromio(d,s,sz)

static inline void 
_memcpy_toio(volatile void *dst, const void *src, __kernel_size_t n)
{
	const char *s = src;
	volatile void *d = dst;

	while (n--) {
		char tmp = *s++;
		writeb(tmp, d);
		d++;
	}
}

#define memcpy_toio(d,s,sz)	_memcpy_toio(d,s,sz)


#endif /* !(__SPARC_IO_H) */
