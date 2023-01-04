#ifndef lint
static  char sccsid[] = "@(#)es_attr.c 1.4 87/01/07";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Attribute support for entity streams.
 */

#include <varargs.h>
#include <sys/types.h>
#include <sunwindow/attr.h>
#include "primal.h"

#include "entity_stream.h"

/* VARARGS1 */
extern int
es_set(esh, va_alist)
	register Es_handle	 esh;
	va_dcl
{
	caddr_t			attr_argv[ATTR_STANDARD_SIZE];
	va_list			args;

	va_start(args);
	(void) attr_make(attr_argv, ATTR_STANDARD_SIZE, args);
	va_end(args);
	return(esh->ops->set(esh, attr_argv));
}
