#ifndef lint
static	char sccsid[] = "@(#)close.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

extern vti;
closevt(){
	close(vti);
}
closepl(){
	close(vti);
}
