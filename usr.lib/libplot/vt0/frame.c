#ifndef lint
static	char sccsid[] = "@(#)frame.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

frame(n)
{
	extern vti;
	n=n&0377 | 02000;
	write(vti,&n,2);
}
