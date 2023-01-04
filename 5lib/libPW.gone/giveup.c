#ifndef lint
static	char sccsid[] = "@(#)giveup.c 1.1 86/09/24 SMI"; /* from S5R2 3.2 */
#endif

/*
	Chdir to "/" if argument is 0.
	Set IOT signal to system default.
	Call abort(III).
	(Shouldn't produce a core when called with a 0 argument.)
*/

# include "signal.h"

giveup(dump)
{
	if (!dump)
		chdir("/");
	signal(SIGIOT,SIG_DFL);
	abort();
}
