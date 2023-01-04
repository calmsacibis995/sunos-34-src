#ifndef lint
static	char sccsid[] = "@(#)zs_proto.c 1.1 86/09/25 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "zs.h"
#if NZS > 0
#include "../h/types.h"
#include "../sundev/zscom.h"

extern struct zsops zsops_null;
extern struct zsops zsops_async;

struct zsops *zs_proto[] = {
	&zsops_null,			/* must be first */
	&zsops_async,
	/* new entries go here */
	0,				/* must be last */
};
#endif
