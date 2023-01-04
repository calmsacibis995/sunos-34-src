#ifndef lint
static	char sccsid[] = "@(#)selection.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - selection handling
 */

#include <stdio.h>
#include <ctype.h>
#include <suntool/tool_hs.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/textsw.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>
#include <sunwindow/notify.h>
#include "glob.h"
#include "tool.h"

int	mt_last_sel_msg;

static Seln_request    *seln_buffer;
static Seln_holder	seln_holder;

/*
 * Check for selection service.
 */
mt_mail_seln_exists()
{
	Seln_holder holder;
	
	holder = seln_inquire(SELN_PRIMARY);
	if (holder.rank == SELN_UNKNOWN){
		(void)fprintf(stderr, "%s: can't find selection service\n", mt_cmdname);
		return (0);
	} else
		return (1);
}

/*
 * Return the number of the message the current selection refers to.
 */
int
mt_get_curselmsg()
{
	int n;

	seln_holder = seln_inquire(SELN_PRIMARY);
	/* does the headersw have the primary selection? */
	if (seln_holder.state == SELN_NONE ||
	    !seln_holder_same_client(&seln_holder, mt_headersw)) {
		/* no, restore the last selection and use it */
		mt_set_curselmsg(mt_last_sel_msg);
		n = mt_last_sel_msg;
	} else {
		/* yes, get the current selection */
		seln_buffer = seln_ask((Seln_request *)&seln_holder,
		    SELN_REQ_FAKE_LEVEL, SELN_LEVEL_LINE,
		    SELN_REQ_FIRST_UNIT, 0,
		    0);
		if (seln_buffer->status == SELN_FAILED){
			(void)fprintf(stderr, "%s: can't get current selection\n",
			    mt_cmdname);
                        return (0);
		}
		n = mt_lineno_to_msg(mt_seln_get_lineno(seln_buffer) + 1);
	}
	if (n <= 0 || n > mt_maxmsg || mt_message[n].m_deleted) {
		mt_warn("Illegal message number.");
		return (0);
	}
	if (mt_value("allowreversescan")) {
		if (n < mt_curmsg)
			mt_scandir = -1;
		else if (n > mt_curmsg)
			mt_scandir = 1;
	}
	mt_last_sel_msg = n;
	return (n);
}

/*
 * Get the line number within the header
 * file of the current selection.
 */
int
mt_seln_get_lineno(buffer)
	Seln_request *buffer;
{
	unsigned *srcp;

	srcp = (unsigned *)(LINT_CAST(buffer->data));
	if (*srcp++ != (unsigned)SELN_REQ_FAKE_LEVEL)
		return (-1);
	srcp++;
	if (*srcp++ != (unsigned)SELN_REQ_FIRST_UNIT)
		return (-1);
	return ((int)*srcp);
}

/*
 * Convert a line number in the header
 * window into a message number.
 */
mt_lineno_to_msg(lineno)
	int lineno;
{
	register int i;
	register struct msg *mp;

	if (lineno < 0)
		return (0);
	for (i = 1, mp = &mt_message[1]; i <= mt_maxmsg; i++, mp++) {
		if (mp->m_deleted)
			continue;
		if (--lineno == 0)
			return (i);
	}
	return (0);
}
