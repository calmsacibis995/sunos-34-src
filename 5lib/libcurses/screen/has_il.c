#ifndef lint
static	char sccsid[] = "@(#)has_il.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * Queries: does the terminal have insert/delete line?
 */
has_il()
{
	return insert_line && delete_line || change_scroll_region;
}
