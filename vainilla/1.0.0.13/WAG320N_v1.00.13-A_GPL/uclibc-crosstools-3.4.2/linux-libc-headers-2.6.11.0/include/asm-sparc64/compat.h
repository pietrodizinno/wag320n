#ifndef _ASM_SPARC64_COMPAT_H
#define _ASM_SPARC64_COMPAT_H
/*
 * Architecture specific compatibility types
 */
#include <linux/types.h>

#define COMPAT_USER_HZ	100

typedef __u32		compat_size_t;
typedef __s32		compat_ssize_t;
typedef __s32		compat_time_t;
typedef __s32		compat_clock_t;
typedef __s32		compat_pid_t;
typedef __u16		compat_uid_t;
typedef __u16		compat_gid_t;
typedef __u16		compat_mode_t;
typedef __u32		compat_ino_t;
typedef __u16		compat_dev_t;
typedef __s32		compat_off_t;
typedef __s64		compat_loff_t;
typedef __s16		compat_nlink_t;
typedef __u16		compat_ipc_pid_t;
typedef __s32		compat_daddr_t;
typedef __u32		compat_caddr_t;
typedef __kernel_fsid_t	compat_fsid_t;
typedef __s32		compat_key_t;

typedef __s32		compat_int_t;
typedef __s32		compat_long_t;
typedef __u32		compat_uint_t;
typedef __u32		compat_ulong_t;

struct compat_timespec {
	compat_time_t	tv_sec;
	__s32		tv_nsec;
};

struct compat_timeval {
	compat_time_t	tv_sec;
	__s32		tv_usec;
};

struct compat_stat {
	compat_dev_t	st_dev;
	compat_ino_t	st_ino;
	compat_mode_t	st_mode;
	compat_nlink_t	st_nlink;
	compat_uid_t	st_uid;
	compat_gid_t	st_gid;
	compat_dev_t	st_rdev;
	compat_off_t	st_size;
	compat_time_t	st_atime;
	__u32		__unused1;
	compat_time_t	st_mtime;
	__u32		__unused2;
	compat_time_t	st_ctime;
	__u32		__unused3;
	compat_off_t	st_blksize;
	compat_off_t	st_blocks;
	__u32		__unused4[2];
};

struct compat_flock {
	short		l_type;
	short		l_whence;
	compat_off_t	l_start;
	compat_off_t	l_len;
	compat_pid_t	l_pid;
	short		__unused;
};

#define F_GETLK64	12
#define F_SETLK64	13
#define F_SETLKW64	14

struct compat_flock64 {
	short		l_type;
	short		l_whence;
	compat_loff_t	l_start;
	compat_loff_t	l_len;
	compat_pid_t	l_pid;
	short		__unused;
};

struct compat_statfs {
	int		f_type;
	int		f_bsize;
	int		f_blocks;
	int		f_bfree;
	int		f_bavail;
	int		f_files;
	int		f_ffree;
	compat_fsid_t	f_fsid;
	int		f_namelen;	/* SunOS ignores this field. */
	int		f_frsize;
	int		f_spare[5];
};

#define COMPAT_RLIM_INFINITY 0x7fffffff

typedef __u32		compat_old_sigset_t;

#define _COMPAT_NSIG		64
#define _COMPAT_NSIG_BPW	32

typedef __u32		compat_sigset_word;

#define COMPAT_OFF_T_MAX	0x7fffffff
#define COMPAT_LOFF_T_MAX	0x7fffffffffffffffL

/*
 * A pointer passed in from user mode. This should not
 * be used for syscall parameters, just declare them
 * as pointers because the syscall entry code will have
 * appropriately comverted them already.
 */
typedef	__u32		compat_uptr_t;

static inline void *compat_ptr(compat_uptr_t uptr)
{
	return (void *)(unsigned long)uptr;
}

static inline compat_uptr_t ptr_to_compat(void *uptr)
{
	return (__u32)(unsigned long)uptr;
}

static __inline__ void *compat_alloc_user_space(long len)
{
	struct pt_regs *regs = current_thread_info()->kregs;
	unsigned long usp = regs->u_regs[UREG_I6];

	if (!(test_thread_flag(TIF_32BIT)))
		usp += STACK_BIAS;
	else
		usp &= 0xffffffffUL;

	return (void *) (usp - len);
}

struct compat_ipc64_perm {
	compat_key_t key;
	__kernel_uid_t uid;
	__kernel_gid_t gid;
	__kernel_uid_t cuid;
	__kernel_gid_t cgid;
	unsigned short __pad1;
	compat_mode_t mode;
	unsigned short __pad2;
	unsigned short seq;
	unsigned long __unused1;	/* yes they really are 64bit pads */
	unsigned long __unused2;
};

struct compat_semid64_ds {
	struct compat_ipc64_perm sem_perm;
	unsigned int	__pad1;
	compat_time_t	sem_otime;
	unsigned int	__pad2;
	compat_time_t	sem_ctime;
	__u32		sem_nsems;
	__u32		__unused1;
	__u32		__unused2;
};

struct compat_msqid64_ds {
	struct compat_ipc64_perm msg_perm;
	unsigned int	__pad1;
	compat_time_t	msg_stime;
	unsigned int	__pad2;
	compat_time_t	msg_rtime;
	unsigned int	__pad3;
	compat_time_t	msg_ctime;
	unsigned int	msg_cbytes;
	unsigned int	msg_qnum;
	unsigned int	msg_qbytes;
	compat_pid_t	msg_lspid;
	compat_pid_t	msg_lrpid;
	unsigned int	__unused1;
	unsigned int	__unused2;
};

struct compat_shmid64_ds {
	struct compat_ipc64_perm shm_perm;
	unsigned int	__pad1;
	compat_time_t	shm_atime;
	unsigned int	__pad2;
	compat_time_t	shm_dtime;
	unsigned int	__pad3;
	compat_time_t	shm_ctime;
	compat_size_t	shm_segsz;
	compat_pid_t	shm_cpid;
	compat_pid_t	shm_lpid;
	unsigned int	shm_nattch;
	unsigned int	__unused1;
	unsigned int	__unused2;
};

#endif /* _ASM_SPARC64_COMPAT_H */
