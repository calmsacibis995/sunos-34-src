#ifndef lint
static	char sccsid[] = "@(#)move.c 1.1 86/09/25 SMI"; /* from UCB 4.1 6/27/83 */
#endif

move(xi,yi){
		movep(xconv(xsc(xi)),yconv(ysc(yi)));
		return;
}
