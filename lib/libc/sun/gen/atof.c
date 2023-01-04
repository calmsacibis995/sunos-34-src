#ifdef sccsid
static  char sccsid[] = "@(#)atof.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

double atof( p )
char *p ;
{
double strtod() ;

return(strtod(p,(char **) 0)) ;
}
