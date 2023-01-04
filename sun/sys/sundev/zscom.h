/*    @(#)zscom.h 1.1 86/09/25 SMI      */

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * Support common to all users of Z8530 devices
 */

/*
 * Interrupt vectors - per protocol
 */
struct zsops {
	int	(*zsop_attach)();	/* attach protocol */
	int	(*zsop_txint)();	/* xmit buffer empty */
	int	(*zsop_xsint)();	/* external/status */
	int	(*zsop_rxint)();	/* receive char available */
	int	(*zsop_srint)();	/* special receive condition */
	int	(*zsop_softint)();	/* process software interrupt */
};

/*
 * Common data
 */
struct zscom {
	int	(*zs_vec[4])();		/* vector routines - must be first */
	struct zscc_device *zs_addr;	/* address of half of chip  - second */
	short	zs_unit;		/* which channel (0:NZSLINE) */
	caddr_t	zs_priv;		/* protocol private data */
	struct zsops *zs_ops;		/* intr op vector */
	u_char	zs_wreg[16];		/* shadow of write registers */
	char	zs_flags;		/* random flags */
};
/* flags */
#define	ZS_NEEDSOFT	1

int zssoftpend;				/* level 3 interrupt pending */
#define	ZSSETSOFT(zs)	{		\
	zs->zs_flags |= ZS_NEEDSOFT;	\
	if (!zssoftpend) {		\
		zssoftpend = 1;		\
		setzssoft();		\
	}				\
}

/* 
 * Macros to access a port
 */
#define	ZREAD(n)	zszread(zs->zs_addr, n)
#define	ZWRITE(n,v)	zszwrite(zs->zs_addr, n, (int)(zs->zs_wreg[n] = (v)))
#define	ZBIS(n,v)	zszwrite(zs->zs_addr, n, (int)(zs->zs_wreg[n] |= (v)))
#define	ZBIC(n,v)	zszwrite(zs->zs_addr, n, (int)(zs->zs_wreg[n] &=~ (v)))

extern struct zsops *zs_proto[];
