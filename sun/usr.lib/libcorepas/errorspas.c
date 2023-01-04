#ifndef lint
static char sccsid[] = "@(#)errorspas.c 1.1 86/09/25 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

int print_error();
int report_most_recent_error();

int printerror(f77string, error)
char *f77string;
int error;
	{
	char *sptr;
	char pasarg[257];
	int i,strlen;

	strlen = 256;
	sptr = f77string+256;
	while ((*--sptr) == ' ') {strlen--;};
	strncpy (pasarg,f77string,strlen);
	pasarg[strlen] = '\0';
	i = print_error(pasarg, error);
	return(i);
	}

int reportrecenterr(error)
int *error;
	{
	return(report_most_recent_error(error));
	}
