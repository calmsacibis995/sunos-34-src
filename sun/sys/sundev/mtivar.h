/*	@(#)mtivar.h 1.1 86/09/25 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Systech MTI-800/1600 Multiple Terminal Interface driver
 */

struct	mti_softc {
	int	msc_have;	/* number of response chars accumulated */
	u_char	msc_rbuf[8];	/* buffer for responses */
	struct	clist msc_cmdq;	/* queue of commands if cmd fifo is busy */
};
