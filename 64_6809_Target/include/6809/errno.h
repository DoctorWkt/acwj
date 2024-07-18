#ifndef __ERRNO_H
#define __ERRNO_H

/*
 * Error codes
 */
#define EPERM           1               /* Not owner */
#define ENOENT          2               /* No such file or directory */
#define ESRCH           3               /* No such process */
#define EINTR           4               /* Interrupted System Call */
#define EIO             5               /* I/O Error */
#define ENXIO           6               /* No such device or address */
#define E2BIG           7               /* Arg list too long */
#define ENOEXEC         8               /* Exec format error */
#define EBADF           9               /* Bad file number */
#define ECHILD          10              /* No children */
#define EAGAIN          11              /* No more processes */
#define ENOMEM          12              /* Not enough core */
#define EACCES          13              /* Permission denied */
#define EFAULT          14              /* Bad address */
#define ENOTBLK         15              /* Block device required */
#define EBUSY           16              /* Mount device busy */
#define EEXIST          17              /* File exists */
#define EXDEV           18              /* Cross-device link */
#define ENODEV          19              /* No such device */
#define ENOTDIR         20              /* Not a directory */
#define EISDIR          21              /* Is a directory */
#define EINVAL          22              /* Invalid argument */
#define ENFILE          23              /* File table overflow */
#define EMFILE          24              /* Too many open files */
#define ENOTTY          25              /* Not a typewriter */
#define ETXTBSY         26              /* Text file busy */
#define EFBIG           27              /* File too large */
#define ENOSPC          28              /* No space left on device */
#define ESPIPE          29              /* Illegal seek */
#define EROFS           30              /* Read-only file system */
#define EMLINK          31              /* Too many links */
#define EPIPE           32              /* Broken pipe */

/* math software */
#define EDOM            33              /* Argument too large */
#define ERANGE          34              /* Result too large */

#define EWOULDBLOCK	EAGAIN		/* Operation would block */
#define ENOLOCK		35		/* Lock table full */
#define ENOTEMPTY	36		/* Directory is not empty */
#define ENAMETOOLONG    37              /* File name too long */
#define EAFNOSUPPORT	38		/* Address family not supported */
#define EALREADY	39		/* Operation already in progress */
#define EADDRINUSE	40		/* Address already in use */
#define EADDRNOTAVAIL	41		/* Address not available */
#define ENOSYS		42		/* No such system call */
#define EPFNOSUPPORT	43		/* Protocol not supported */
#define EOPNOTSUPP	44		/* Operation not supported on transport endpoint */
#define ECONNRESET	45		/* Connection reset by peer */
#define ENETDOWN	46		/* Network is down */
#define EMSGSIZE	47		/* Message too long */

#define ETIMEDOUT	48		/* Connection timed out */
#define ECONNREFUSED	49		/* Connection refused */
#define EHOSTUNREACH	50		/* No route to host */
#define EHOSTDOWN	51		/* Host is down */
#define	ENETUNREACH	52		/* Network is unreachable */
#define ENOTCONN	53		/* Transport endpoint is not connected */
#define EINPROGRESS	54		/* Operation now in progress */
#define ESHUTDOWN	55		/* Cannot send after transport endpoint shutdown */
#define EISCONN         56              /* Socket is already connected */
#define EDESTADDRREQ    57              /* No destination address specified */
#define ENOBUFS		58		/* No buffer space available */
#define EPROTONOSUPPORT	59		/* Protocol not supported */
#define __ERRORS	56

extern int sys_nerr;
extern char *sys_errlist[];
extern int errno;

#endif

