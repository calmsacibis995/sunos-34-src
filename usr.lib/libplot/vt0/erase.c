#ifndef lint
static	char sccsid[] = "@(#)erase.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

extern vti;
erase(){
	int i;
	i=0401;
	write(vti,&i,2);
}
