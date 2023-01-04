#ifndef lint
static  char sccsid[] = "@(#)xy_conf.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include "xy.h"

/*
 * Config dependent data structures for the Xylogics 450 driver.
 */
#include "../h/param.h"
#include "../h/buf.h"
#include "../sundev/mbvar.h"
#include "../sundev/xyvar.h"

int nxy = NXY;			/* So the driver can use these defines */
int nxyc = NXYC;

/*
 * Unit structures.
 */
struct xyunit xyunits[NXY];

/*
 * Controller structures.
 */
struct xyctlr xyctlrs[NXYC];

/*
 * Generic structures.
 */
struct	mb_ctlr *xycinfo[NXYC];
struct	mb_device *xydinfo[NXY];

