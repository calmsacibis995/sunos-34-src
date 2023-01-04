/*	@(#)ioccom.h 1.1 86/09/24 SMI	*/

#ifndef _IO
/*
 * Ioctl's have the command encoded in the lower word,
 * and the size of any in or out parameters in the upper
 * word.  The high 2 bits of the upper word are used
 * to encode the in/out status of the parameter; for now
 * we restrict parameters to at most 128 bytes.
 */
#define	IOCPARM_MASK	0x7f		/* parameters must be < 128 bytes */
#define	IOC_VOID	0x20000000	/* no parameters */
#define	IOC_OUT		0x40000000	/* copy out parameters */
#define	IOC_IN		0x80000000	/* copy in parameters */
#define	IOC_INOUT	(IOC_IN|IOC_OUT)
/* the 0x20000000 is so we can distinguish new ioctl's from old */
#define	_IO(x,y)	(IOC_VOID|('x'<<8)|y)
#define	_IOR(x,y,t)	(IOC_OUT|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#define	_IORN(x,y,t)	(IOC_OUT|(((t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#define	_IOW(x,y,t)	(IOC_IN|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#define	_IOWN(x,y,t)	(IOC_IN|(((t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
/* this should be _IORW, but stdio got there first */
#define	_IOWR(x,y,t)	(IOC_INOUT|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#endif
