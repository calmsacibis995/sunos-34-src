

#ifndef lint
static  char sccsid[] = "@(#)pw_dbl.c 1.2 87/02/24 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Pw_dbl.c: Implement double buffering 
 */
#include <varargs.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdio.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/pw_dblbuf.h>
#include <pixrect/pr_dblbuf.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_ioctl.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_lock.h>

int pw_dbl_get();
void pw_dbl_access();
void pw_dbl_release();
void pw_dbl_flip();
int pw_dbl_set();


/* Request for access to the double buffering */
void 
pw_dbl_access(pw)
	Pixwin *pw;
{
	if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {
		pw->pw_clipdata->pwcd_flags |= PWCD_DBLACCESSED;
		(void)werror(ioctl(pw->pw_windowfd, WINDBLACCESS, 0),
			     WINDBLACCESS);
	}
}


/* Release the double buffer accessing */
void
 pw_dbl_release(pw)
	struct pixwin	*pw;
{
	if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {
		pw->pw_clipdata->pwcd_flags &= ~PWCD_DBLACCESSED;
		(void)werror(ioctl(pw->pw_clipdata->pwcd_windowfd, WINDBLRLSE,
			0), WINDBLRLSE);
	}
}


/* Flip the write control bits */
void 
pw_dbl_flip(pw)
	Pixwin	*pw;
{
	if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL &&
	    pw->pw_clipdata->pwcd_flags & PWCD_DBLACCESSED)
		(void)werror(ioctl(pw->pw_windowfd, WINDBLFLIP, 0), WINDBLFLIP);
}



/* pw_dbl_get returns the current value of which_attr. */
int
pw_dbl_get(pw, which_attr)
	struct pixwin		*pw;
	Pw_dbl_attribute		which_attr;
{
	struct pwset list;

	if (which_attr == PW_DBL_AVAIL)
		return ((pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL)? TRUE:
			FALSE);
	if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {
		switch (which_attr) {
		case PW_DBL_DISPLAY: 
			list.attribute = PR_DBL_DISPLAY;
			break;

		case PW_DBL_WRITE: 
			list.attribute = PR_DBL_WRITE;
			break;

		case PW_DBL_READ:
			list.attribute = PR_DBL_READ;
			break;
		default:
			return(PW_DBL_ERROR);
		
		}
		(void)werror(ioctl(pw->pw_windowfd, WINDBLGET, &list),
			     WINDBLGET);
		return(list.value);
	} else
		return(PW_DBL_ERROR);

} /* pw_dbl_get */


/* Set double buffer control bits */
int
pw_dbl_set(pw, va_alist)
	struct pixwin	*pw;
	va_dcl
{
	va_list valist;
	Attr_avlist avlist;
	caddr_t vlist[ATTR_STANDARD_SIZE];
	struct pwset pw_setlist;
	Pw_dbl_attribute which_attr;

	if (pw->pw_clipdata->pwcd_flags & PWCD_DBL_AVAIL) {
		va_start(valist);
		avlist = attr_make(vlist, ATTR_STANDARD_SIZE, valist);
		va_end(valist);
		while (which_attr = (Pw_dbl_attribute) *avlist++ ) {
			if ( which_attr == PW_DBL_DISPLAY ) 
				return(PW_DBL_ERROR);
			else {
				pw_setlist.value = (int) *avlist++;
				if ( which_attr == PW_DBL_READ )
					pw_setlist.attribute = PR_DBL_READ;
				else  if ( which_attr == PW_DBL_WRITE ) 
					 pw_setlist.attribute = PR_DBL_WRITE;
				else return(PW_DBL_ERROR);
				(void)werror(ioctl(pw->pw_windowfd, WINDBLSET, 
						&pw_setlist), WINDBLSET);
			}
		}
		return(TRUE);
	} else
		return(FALSE);
}
