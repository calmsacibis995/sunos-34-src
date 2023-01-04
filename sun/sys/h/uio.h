/*	@(#)uio.h 1.1 86/09/25 SMI; from UCB 4.1 82/09/04	*/

struct iovec {
	caddr_t	iov_base;
	int	iov_len;
};

struct uio {
	struct	iovec *uio_iov;
	int	uio_iovcnt;
	int	uio_offset;
	short	uio_seg;
	short	uio_fmode;
	int	uio_resid;
};

enum	uio_rw { UIO_READ, UIO_WRITE };

/*
 * segments
 */
#define UIOSEG_USER	0
#define UIOSEG_KERNEL	1
