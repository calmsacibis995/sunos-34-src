#ifndef lint
static	char sccsid[] = "@(#)nttychktrm.c 1.3 86/12/13 SMI"; /* from S5R2 1.1 */
#endif

#include "curses.ext"

char *_c_why_not = NULL;
static char *
_stupid = "Sorry, I don't know how to deal with your '%s' terminal.\r\n";
static char *
_unknown = "Sorry, I need to know a more specific terminal type than '%s'.\r\n";
static char *
_noterminal = "Sorry, I don't know anything about your '%s' terminal.\r\n";
#ifdef S5EMUL
static char *
_nodatabase = "Sorry, I can't find /usr/5lib/terminfo.\r\n";
#else
static char *
_nodatabase = "Sorry, I can't find /usr/lib/terminfo.\r\n";
#endif
static char *
_bad_malloc = "Sorry, I can't get enough memory.\r\n";

struct screen *
_new_tty(type, fd)
char	*type;
FILE	*fd;
{
	int retcode;
	struct map *_init_keypad();
	char *calloc();

#ifdef DEBUG
	if(outf) fprintf(outf, "__new_tty: type %s, fd %x\n", type, fd);
#endif
	/*
	 * Allocate an SP structure if there is none, or if SP is
	 * still pointing to an old structure from a previous call
	 * to this routine.  But don't allocate one if we are being
	 * called from a higher level curses routine.  Since it's our
	 * job to initialize the phys_scr field and higher level routines
	 * aren't supposed to, we check that field to figure which to do.
	 */
	if (SP == NULL || SP->cur_body!=0)
		SP = (struct screen *) calloc(1, sizeof (struct screen));
	if (SP == NULL) {
		_c_why_not = _bad_malloc;
		return NULL;
	}
	SP->term_file = fd;
	if (type == 0)
		type = "unknown";
	_setbuffered(fd);
	setupterm(type, fileno(fd), &retcode);
	savetty();	/* as a "useful default" - hanging up is nasty. */

	/*
	 * This happens if /usr/lib/terminfo doesn't exist, there is
	 * no such terminal type, or the file is corrupted.
	 * This would be a good place to print an error message.
	 */
	if (retcode == 0) {
		_c_why_not = _noterminal;
		return NULL;
	} else if (retcode < 0) {
		_c_why_not = _nodatabase;
		return NULL;
	}
	if (chk_trm() == ERR)
		return NULL;
	SP->tcap = cur_term;
	SP->doclear = 1;
	SP->cur_body = (struct line **) calloc(lines+2, sizeof (struct line *));
	SP->std_body = (struct line **) calloc(lines+2, sizeof (struct line *));
	if (SP->cur_body == NULL || SP->std_body == NULL) {
		_c_why_not = _bad_malloc;
		return NULL;
	}
#ifdef KEYPAD
	SP->kp = _init_keypad();
#endif
	SP->input_queue = (short *) calloc(20, sizeof (short));
	if (SP->input_queue == NULL) {
		_c_why_not = _bad_malloc;
		return NULL;
	}
	SP->input_queue[0] = -1;

	SP->virt_x = 0;		/* X and Y coordinates of the SP->curptr */
	SP->virt_y = 0;		/* between updates. */
	_init_costs();
	return SP;
}

static int
chk_trm()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "chk_trm().\n");
#endif

	if (generic_type) {
		_c_why_not = _unknown;
		return ERR;
	}
	if (clear_screen == 0 || hard_copy || over_strike) {
		_c_why_not = _stupid;
		return ERR;
	}
	if (cursor_address) {
		/* we can handle it */
	} else 
	if ( (cursor_up || cursor_home)	/* some way to move up */
	    && cursor_down		/* some way to move down */
	    && (cursor_left || carriage_return)	/* ... move left */
					/* printing chars moves right */
		) {
		/* we can handle it */
	} else {
		_c_why_not = _stupid;
		return ERR;
	}
	return OK;
}
