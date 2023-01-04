#ifndef lint
static	char sccsid[] = "@(#)userexit.c 1.1 86/09/24 SMI"; /* from S5R2 3.1 */
#endif

/*
	Default userexit routine for fatal and setsig.
	User supplied userexit routines can be used for logging.
*/

userexit(code)
{
	return(code);
}
