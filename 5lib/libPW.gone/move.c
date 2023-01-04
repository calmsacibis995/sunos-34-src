#ifndef lint
static	char sccsid[] = "@(#)move.c 1.1 86/09/24 SMI"; /* from S5R2 3.1 */
#endif

/*
	Copies `n' characters from string `a' to string `b'.
*/

char *move(a,b,n)
char *a,*b;
unsigned n;
{
	/*
		Test for non-zero number of characters to move
	*/
	if (n != 0)
		(void) memcpy(b, a, (int)n);
}
