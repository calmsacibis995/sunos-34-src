#ifndef lint
static	char sccsid[] = "@(#)typeahead.c 1.1 86/09/24 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

/*
 * Set the file descriptor for typeahead checks to fd.  fd can be -1
 * to disable the checking.
 */
typeahead(fd)
int fd;
{
	SP->check_fd = fd;
}
