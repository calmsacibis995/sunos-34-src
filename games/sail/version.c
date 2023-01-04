#ifndef lint
static	char sccsid[] = "@(#)version.c 1.1 86/09/24 SMI"; /* from UCB 1.1 83/03/19 */
#endif
version()
{
    static char *cp =
	"Wooden Ships and Iron Men, Version 1.1 (83/03/19)";

    Signal(cp, 0, 0);
    return;
}
