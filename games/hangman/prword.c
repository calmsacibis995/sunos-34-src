#ifndef lint
static	char sccsid[] = "@(#)prword.c 1.1 86/09/24 SMI"; /* from UCB */
#endif
# include	"hangman.h"

/*
 * prword:
 *	Print out the current state of the word
 */
prword()
{
	move(KNOWNY, KNOWNX + sizeof "Word: ");
	addstr(Known);
	clrtoeol();
}
