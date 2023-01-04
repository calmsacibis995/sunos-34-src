#ifndef lint
static  char sccsid[] = "@(#)white_space.c 1.3 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "string_utils.h"
/*
 * these two procedures are in a separate file because they are also defined
 * in libio_stream.a, and having them be in string_utils.c tends to make the
 * loader complain about things being multiply defined 
 */

enum CharClass
white_space(c)
	char            c;
{
	switch (c) {
	    case ' ':
	    case '\n':
	    case '\t':
		return (Sepr);
	    default:
		return (Other);
	}
}

/* ARGSUSED */
struct CharAction
everything(c)
	char            c;
{
	static struct CharAction val = {False, True};

	return (val);
}
