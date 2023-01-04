#ifndef lint
static	char sccsid[] = "@(#)pause.c 1.1 86/09/24 SMI"; /* from UCB 4.1 83/06/09 */
#endif

/*
 * Backwards compatible pause.
 */
pause()
{

	sigpause(sigblock(0));
}
