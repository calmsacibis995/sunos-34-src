#ifndef lint
static	char sccsid[] = "@(#)mti_conf.c 1.1 86/09/25 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Systech MTI-800/1600 Multiple Terminal Interface driver
 *
 * Configuration dependent variables
 */

#include "mti.h"
#include "../h/param.h"
#include "../h/buf.h"
#include "../h/tty.h"
#include "../sundev/mbvar.h"
#include "../sundev/mtivar.h"

#define	NMTILINE	(NMTI*16)

int	nmti = NMTILINE;

struct	mb_device *mtiinfo[NMTI];

struct	tty mti_tty[NMTILINE];

u_short	mtisoftCAR[NMTI];
u_char	mtiwbits[NMTILINE];	/* copy of writable modem control bits */
u_char	mtirbits[NMTILINE];	/* copy of readable modem control bits */
char	*mtibuf[NMTILINE];	/* pointers to DMA buffers */

struct	mti_softc mti_softc[NMTI];
