#ifndef lint
static	char sccsid[] = "@(#)has_ic.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * Does it have insert/delete char?
 */
has_ic()
{
	return insert_character || enter_insert_mode;
}
