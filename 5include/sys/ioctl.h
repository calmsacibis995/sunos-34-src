/*	@(#)ioctl.h 1.3 87/01/02 SMI; from S5R2 6.1	*/

/*
 * Ioctl definitions
 */
#ifndef	_IOCTL_
#define	_IOCTL_

#include <sys/ttold.h>

#define	FIOCLEX		_IO(f, 1)		/* set exclusive use on fd */
#define	FIONCLEX	_IO(f, 2)		/* remove exclusive use */
/* another local */
#define	FIONREAD	_IOR(f, 127, int)	/* get # bytes to read */
#define	FIONBIO		_IOW(f, 126, int)	/* set/clear non-blocking i/o */
#define	FIOASYNC	_IOW(f, 125, int)	/* set/clear async i/o */
#define	FIOSETOWN	_IOW(f, 124, int)	/* set owner */
#define	FIOGETOWN	_IOR(f, 123, int)	/* get owner */

/* socket i/o controls */
#define	SIOCSHIWAT	_IOW(s,  0, int)		/* set high watermark */
#define	SIOCGHIWAT	_IOR(s,  1, int)		/* get high watermark */
#define	SIOCSLOWAT	_IOW(s,  2, int)		/* set low watermark */
#define	SIOCGLOWAT	_IOR(s,  3, int)		/* get low watermark */
#define	SIOCATMARK	_IOR(s,  7, int)		/* at oob mark? */
#define	SIOCSPGRP	_IOW(s,  8, int)		/* set process group */
#define	SIOCGPGRP	_IOR(s,  9, int)		/* get process group */

#define	SIOCADDRT	_IOW(r, 10, struct rtentry)	/* add route */
#define	SIOCDELRT	_IOW(r, 11, struct rtentry)	/* delete route */

#define	SIOCSIFADDR	_IOW(i, 12, struct ifreq)	/* set ifnet address */
#define	SIOCGIFADDR	_IOWR(i,13, struct ifreq)	/* get ifnet address */
#define	SIOCSIFDSTADDR	_IOW(i, 14, struct ifreq)	/* set p-p address */
#define	SIOCGIFDSTADDR	_IOWR(i,15, struct ifreq)	/* get p-p address */
#define	SIOCSIFFLAGS	_IOW(i, 16, struct ifreq)	/* set ifnet flags */
#define	SIOCGIFFLAGS	_IOWR(i,17, struct ifreq)	/* get ifnet flags */
#define	SIOCSIFMEM	_IOW(i, 18, struct ifreq)	/* set interface mem */
#define	SIOCGIFMEM	_IOWR(i,19, struct ifreq)	/* get interface mem */
#define	SIOCGIFCONF	_IOWR(i,20, struct ifconf)	/* get ifnet list */
#define	SIOCSIFMTU	_IOW(i, 21, struct ifreq)	/* set if_mtu */
#define	SIOCGIFMTU	_IOWR(i,22, struct ifreq)	/* get if_mtu */

	/* from 4.3BSD */
#define	SIOCGIFBRDADDR	_IOWR(i,23, struct ifreq)	/* get broadcast addr */
#define	SIOCSIFBRDADDR	_IOW(i,24, struct ifreq)	/* set broadcast addr */
#define	SIOCGIFNETMASK	_IOWR(i,25, struct ifreq)	/* get net addr mask */
#define	SIOCSIFNETMASK	_IOW(i,26, struct ifreq)	/* set net addr mask */
#define	SIOCGIFMETRIC	_IOWR(i,27, struct ifreq)	/* get IF metric */
#define	SIOCSIFMETRIC	_IOW(i,28, struct ifreq)	/* set IF metric */

#define	SIOCSARP	_IOW(i, 30, struct arpreq)	/* set arp entry */
#define	SIOCGARP	_IOWR(i,31, struct arpreq)	/* get arp entry */
#define	SIOCDARP	_IOW(i, 32, struct arpreq)	/* delete arp entry */
#define SIOCUPPER       _IOW(i, 40, struct ifreq)       /* attach upper layer */
#define SIOCLOWER       _IOW(i, 41, struct ifreq)       /* attach lower layer */

/* protocol i/o controls */
#define SIOCSNIT	_IOW(p,  0, struct nit_ioc)	/* set nit modes */
#define SIOCGNIT	_IOWR(p, 1, struct nit_ioc)	/* get nit modes */

#endif
