#ifndef lint
static	char sccsid[] = "@(#)frexp.c 1.1 86/09/24 SMI"; /* from UCB 4.1 80/12/21 */
#endif

/*
 * the call
 *	x = frexp(arg,&exp);
 * must return a double fp quantity x which is <1.0
 * and the corresponding binary exponent "exp".
 * such that
 *	arg = x*2^exp
 */
double
frexp(x, i)
	double x;
	int *i;
{
	int neg, j;

	j = 0;
	neg = 0;
	if (x<0) {
		x = -x;
		neg = 1;
	}
	if (x>1.0)
		while (x>1) {
			j = j+1;
			x = x/2;
		}
	else if (x<0.5)
		while(x<0.5) {
			j = j-1;
			x = 2*x;
		}
	*i = j;
	if(neg)
		x = -x;
	return (x);
}
