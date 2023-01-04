
#ifndef lint
static	char sccsid[] = "@(#)callback.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "setup_runtime.h"

callback(obj, attr, error_state)
Setup_object    *obj;
Setup_attribute	attr;
int		error_state;
{
	Opaque	value;

	if (TEST_ATTR_BIT(obj->status, attr)) {		/* was in error */
		if (! error_state) {			/* now not in error */
			CLR_ATTR_BIT(obj->status, attr);
		}
	}

	if (attr != SETUP_ALL) {
		value = setup_get(obj, attr);
	} else {
		value = NULL;
	}

	if (error_state) {
		(*obj->callback)(obj, attr, value, setup_msgbuf);
	} else {
		(*obj->callback)(obj, attr, value, NULL);
	}
}
