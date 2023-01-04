#ifndef lint
static char sccsid[] = "@(#)errors77.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */

#include "f77strings.h"

int print_error();
int report_most_recent_error();

int printerror_(f77string, error, f77strleng)
char *f77string;
int *error, f77strleng;
	{
	char *sptr;

	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *f77string++;
	*sptr = '\0';
	return(print_error(_core_77string, *error));
	}

int reportrecenterr_(error)
int *error;
	{
	return(report_most_recent_error(error));
	}
