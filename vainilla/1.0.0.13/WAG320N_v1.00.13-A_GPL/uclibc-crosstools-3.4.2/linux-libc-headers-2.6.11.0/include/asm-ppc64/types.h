#ifndef _PPC64_TYPES_H
#define _PPC64_TYPES_H

#ifndef __ASSEMBLY__

/*
 * This file is never included by application software unless
 * explicitly requested (e.g., via linux/types.h) in which case the
 * application is Linux specific so (user-) name space pollution is
 * not a major issue.  However, for interoperability, libraries still
 * need to be careful to avoid a name clashes.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

typedef unsigned int umode_t;

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

#ifndef __LLH_TYPE_LONG_IS_64BIT__
#define __LLH_TYPE_LONG_IS_64BIT__
#endif
typedef __signed__ long __s64;
typedef unsigned long __u64;

typedef struct {
	__u32 u[4];
} __attribute((aligned(16))) __vector128;

#endif /* __ASSEMBLY__ */

#endif /* _PPC64_TYPES_H */
