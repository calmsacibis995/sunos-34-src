#ifndef lint
static	char sccsid[] = "@(#)logname.c 1.1 86/09/25 SMI"; /* from S5R2 1.3 */
#endif


#include <stdio.h>
main() {
	char *name, *cuserid();

	name = cuserid((char *)NULL);
	if (name == NULL)
		return (1);
	(void) puts (name);
	return (0);
}
