#ifndef lint
static	char sccsid[] = "@(#)substr.c 1.1 86/09/24 SMI"; /* from S5R2 3.1 */
#endif

/*
	Place the `len' length substring of `as' starting at `as[origin]'
	in `aresult'.
	Return `aresult'.
 
  Note: The copying of as to aresult stops if either the
	specified number (len) characters have been copied,
	or if the end of as is found.
	A negative len generally guarantees that everything gets copied.
*/

char *substr(as, aresult, origin, len)
char *as, *aresult;
int origin;
register unsigned len;
{
	register char *s, *result;

	s = as + origin;
	result = aresult;
	++len;
	while (--len && (*result++ = *s++)) ;
	if (len == 0)
		*result = 0;
	return(aresult);
}
