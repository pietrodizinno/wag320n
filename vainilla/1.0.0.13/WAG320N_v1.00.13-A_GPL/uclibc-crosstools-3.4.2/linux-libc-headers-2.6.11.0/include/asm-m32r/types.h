#ifndef _ASM_M32R_TYPES_H
#define _ASM_M32R_TYPES_H

#ifndef __ASSEMBLY__

/* $Id: types.h 5227 2004-10-21 16:04:54Z mmazur $ */

/* orig : i386 2.4.18 */

typedef unsigned short umode_t;

/*
 * __xx is ok: it doesn't pollute the POSIX namespace. Use these in the
 * header files exported to user space
 */

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif
#endif /* __ASSEMBLY__ */

/*
 * These aren't exported outside the kernel to avoid name space clashes
 */
#endif  /* _ASM_M32R_TYPES_H */
