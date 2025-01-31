#ifndef _SPARC64_ERRNO_H
#define _SPARC64_ERRNO_H

#ifndef _LINUX_ERRNO_H
 #include <linux/errno.h>
#endif

#undef	EDEADLK
#undef	ENAMETOOLONG
#undef	ENOLCK
#undef	ENOSYS
#undef	ENOTEMPTY
#undef	ELOOP
#undef	EWOULDBLOCK
#undef	ENOMSG
#undef	EIDRM
#undef	ECHRNG
#undef	EL2NSYNC
#undef	EL3HLT
#undef	EL3RST
#undef	ELNRNG
#undef	EUNATCH
#undef	ENOCSI
#undef	EL2HLT
#undef	EBADE
#undef	EBADR
#undef	EXFULL
#undef	ENOANO
#undef	EBADRQC
#undef	EBADSLT
#undef	EDEADLOCK
#undef	EBFONT
#undef	ENOSTR
#undef	ENODATA
#undef	ETIME
#undef	ENOSR
#undef	ENONET
#undef	ENOPKG
#undef	EREMOTE
#undef	ENOLINK
#undef	EADV
#undef	ESRMNT
#undef	ECOMM
#undef	EPROTO
#undef	EMULTIHOP
#undef	EDOTDOT
#undef	EBADMSG
#undef	EOVERFLOW
#undef	ENOTUNIQ
#undef	EBADFD
#undef	EREMCHG
#undef	ELIBACC
#undef	ELIBBAD
#undef	ELIBSCN
#undef	ELIBMAX
#undef	ELIBEXEC
#undef	EILSEQ
#undef	ERESTART
#undef	ESTRPIPE
#undef	EUSERS
#undef	ENOTSOCK
#undef	EDESTADDRREQ
#undef	EMSGSIZE
#undef	EPROTOTYPE
#undef	ENOPROTOOPT
#undef	EPROTONOSUPPORT
#undef	ESOCKTNOSUPPORT
#undef	EOPNOTSUPP
#undef	EPFNOSUPPORT
#undef	EAFNOSUPPORT
#undef	EADDRINUSE
#undef	EADDRNOTAVAIL
#undef	ENETDOWN
#undef	ENETUNREACH
#undef	ENETRESET
#undef	ECONNABORTED
#undef	ECONNRESET
#undef	ENOBUFS
#undef	EISCONN
#undef	ENOTCONN
#undef	ESHUTDOWN
#undef	ETOOMANYREFS
#undef	ETIMEDOUT
#undef	ECONNREFUSED
#undef	EHOSTDOWN
#undef	EHOSTUNREACH
#undef	EALREADY
#undef	EINPROGRESS
#undef	ESTALE
#undef	EUCLEAN
#undef	ENOTNAM
#undef	ENAVAIL
#undef	EISNAM
#undef	EREMOTEIO
#undef	EDQUOT
#undef	ENOMEDIUM
#undef	EMEDIUMTYPE
#undef	ECANCELED
#undef	ENOKEY
#undef	EKEYEXPIRED
#undef	EKEYREVOKED
#undef	EKEYREJECTED

/* These match the SunOS error numbering scheme. */

#define	EWOULDBLOCK	EAGAIN	/* Operation would block */
#define	EINPROGRESS	36	/* Operation now in progress */
#define	EALREADY	37	/* Operation already in progress */
#define	ENOTSOCK	38	/* Socket operation on non-socket */
#define	EDESTADDRREQ	39	/* Destination address required */
#define	EMSGSIZE	40	/* Message too long */
#define	EPROTOTYPE	41	/* Protocol wrong type for socket */
#define	ENOPROTOOPT	42	/* Protocol not available */
#define	EPROTONOSUPPORT	43	/* Protocol not supported */
#define	ESOCKTNOSUPPORT	44	/* Socket type not supported */
#define	EOPNOTSUPP	45	/* Op not supported on transport endpoint */
#define	EPFNOSUPPORT	46	/* Protocol family not supported */
#define	EAFNOSUPPORT	47	/* Address family not supported by protocol */
#define	EADDRINUSE	48	/* Address already in use */
#define	EADDRNOTAVAIL	49	/* Cannot assign requested address */
#define	ENETDOWN	50	/* Network is down */
#define	ENETUNREACH	51	/* Network is unreachable */
#define	ENETRESET	52	/* Net dropped connection because of reset */
#define	ECONNABORTED	53	/* Software caused connection abort */
#define	ECONNRESET	54	/* Connection reset by peer */
#define	ENOBUFS		55	/* No buffer space available */
#define	EISCONN		56	/* Transport endpoint is already connected */
#define	ENOTCONN	57	/* Transport endpoint is not connected */
#define	ESHUTDOWN	58	/* No send after transport endpoint shutdown */
#define	ETOOMANYREFS	59	/* Too many references: cannot splice */
#define	ETIMEDOUT	60	/* Connection timed out */
#define	ECONNREFUSED	61	/* Connection refused */
#define	ELOOP		62	/* Too many symbolic links encountered */
#define	ENAMETOOLONG	63	/* File name too long */
#define	EHOSTDOWN	64	/* Host is down */
#define	EHOSTUNREACH	65	/* No route to host */
#define	ENOTEMPTY	66	/* Directory not empty */
#define EPROCLIM        67      /* SUNOS: Too many processes */
#define	EUSERS		68	/* Too many users */
#define	EDQUOT		69	/* Quota exceeded */
#define	ESTALE		70	/* Stale NFS file handle */
#define	EREMOTE		71	/* Object is remote */
#define	ENOSTR		72	/* Device not a stream */
#define	ETIME		73	/* Timer expired */
#define	ENOSR		74	/* Out of streams resources */
#define	ENOMSG		75	/* No message of desired type */
#define	EBADMSG		76	/* Not a data message */
#define	EIDRM		77	/* Identifier removed */
#define	EDEADLK		78	/* Resource deadlock would occur */
#define	ENOLCK		79	/* No record locks available */
#define	ENONET		80	/* Machine is not on the network */
#define ERREMOTE        81      /* SunOS: Too many lvls of remote in path */
#define	ENOLINK		82	/* Link has been severed */
#define	EADV		83	/* Advertise error */
#define	ESRMNT		84	/* Srmount error */
#define	ECOMM		85      /* Communication error on send */
#define	EPROTO		86	/* Protocol error */
#define	EMULTIHOP	87	/* Multihop attempted */
#define	EDOTDOT		88	/* RFS specific error */
#define	EREMCHG		89	/* Remote address changed */
#define	ENOSYS		90	/* Function not implemented */

/* The rest have no SunOS equivalent. */
#define	ESTRPIPE	91	/* Streams pipe error */
#define	EOVERFLOW	92	/* Value too large for defined data type */
#define	EBADFD		93	/* File descriptor in bad state */
#define	ECHRNG		94	/* Channel number out of range */
#define	EL2NSYNC	95	/* Level 2 not synchronized */
#define	EL3HLT		96	/* Level 3 halted */
#define	EL3RST		97	/* Level 3 reset */
#define	ELNRNG		98	/* Link number out of range */
#define	EUNATCH		99	/* Protocol driver not attached */
#define	ENOCSI		100	/* No CSI structure available */
#define	EL2HLT		101	/* Level 2 halted */
#define	EBADE		102	/* Invalid exchange */
#define	EBADR		103	/* Invalid request descriptor */
#define	EXFULL		104	/* Exchange full */
#define	ENOANO		105	/* No anode */
#define	EBADRQC		106	/* Invalid request code */
#define	EBADSLT		107	/* Invalid slot */
#define	EDEADLOCK	108	/* File locking deadlock error */
#define	EBFONT		109	/* Bad font file format */
#define	ELIBEXEC	110	/* Cannot exec a shared library directly */
#define	ENODATA		111	/* No data available */
#define	ELIBBAD		112	/* Accessing a corrupted shared library */
#define	ENOPKG		113	/* Package not installed */
#define	ELIBACC		114	/* Can not access a needed shared library */
#define	ENOTUNIQ	115	/* Name not unique on network */
#define	ERESTART	116	/* Interrupted syscall should be restarted */
#define	EUCLEAN		117	/* Structure needs cleaning */
#define	ENOTNAM		118	/* Not a XENIX named type file */
#define	ENAVAIL		119	/* No XENIX semaphores available */
#define	EISNAM		120	/* Is a named type file */
#define	EREMOTEIO	121	/* Remote I/O error */
#define	EILSEQ		122	/* Illegal byte sequence */
#define	ELIBMAX		123	/* Atmpt to link in too many shared libs */
#define	ELIBSCN		124	/* .lib section in a.out corrupted */

#define ENOMEDIUM       125     /* No medium found */
#define EMEDIUMTYPE     126     /* Wrong medium type */
#define	ECANCELED	127	/* Operation Cancelled */
#define	ENOKEY		128	/* Required key not available */
#define	EKEYEXPIRED	129	/* Key has expired */
#define	EKEYREVOKED	130	/* Key has been revoked */
#define	EKEYREJECTED	131	/* Key was rejected by service */

#endif /* !(_SPARC64_ERRNO_H) */
