#ifndef lint
static	char sccsid[] = "@(#)acu.c 1.1 86/09/25 SMI"; /* from UCB 4.10 7/16/83 */
#endif

#include "tip.h"

static acu_t *acu = NOACU;
static int conflag;
static int acuabort();
static acu_t *acutype();
static jmp_buf jmpbuf;
/*
 * Establish connection for tip
 *
 * If DU is true, we should dial an ACU whose type is AT.
 * The phone numbers are in PN, and the call unit is in CU.
 *
 * If the PN is an '@', then we consult the PHONES file for
 *   the phone numbers.  This file is /etc/phones, unless overriden
 *   by an exported shell variable.
 *
 * The data base files must be in the format:
 *	host-name[ \t]*phone-number
 *   with the possibility of multiple phone numbers
 *   for a single host acting as a rotary (in the order
 *   found in the file).
 */
char *
connect()
{
	register char *cp = PN;
	char *phnum, string[256];
	int tried = 0;

	if (!DU)
		return (NOSTR);
	/*
	 * @ =>'s use data base in PHONES environment variable
	 *        otherwise, use /etc/phones
	 */
	if (setjmp(jmpbuf)) {
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
		printf("\ncall aborted\n");
		logent(value(HOST), "", "", "call aborted");
		if (acu != NOACU) {
			boolean(value(VERBOSE)) = FALSE;
			if (conflag)
				disconnect();
			else
				(*acu->acu_abort)();
		}
		delock(uucplock);
		exit(1);
	}
	signal(SIGINT, acuabort);
	signal(SIGQUIT, acuabort);
	if ((acu = acutype(AT)) == NOACU)
		return ("unknown ACU type");
	if (*cp != '@') {
		while (*cp) {
			for (phnum = cp; *cp && *cp != '|'; cp++)
				;
			if (*cp)
				*cp++ = '\0';
			
			if (conflag = (*acu->acu_dialer)(phnum, CU)) {
				logent(value(HOST), phnum, acu->acu_name,
					"call completed");
				return (NOSTR);
			} else
				logent(value(HOST), phnum, acu->acu_name,
					"call failed");
			tried++;
		}
	} else {
		if (phfd == NOFILE) {
			printf("%s: ", PH);
			return ("can't open phone number file");
		}
		rewind(phfd);
		while (fgets(string, sizeof(string), phfd) != NOSTR) {
			if (string[0] == '#')
				continue;
			for (cp = string; !any(*cp, " \t\n"); cp++)
				;
			if (*cp == '\n')
				return ("unrecognizable host name");
			*cp++ = '\0';
			if (strcmp(string, value(HOST)))
				continue;
			while (any(*cp, " \t"))
				cp++;
			if (*cp == '\n')
				return ("missing phone number");
			for (phnum = cp; *cp && *cp != '|'; cp++)
				;
			*cp = '\0';
			
			if (conflag = (*acu->acu_dialer)(phnum, CU)) {
				logent(value(HOST), phnum, acu->acu_name,
					"call completed");
				return (NOSTR);
			} else
				logent(value(HOST), phnum, acu->acu_name,
					"call failed");
			tried++;
		}
	}
	if (!tried)
		logent(value(HOST), "", acu->acu_name, "missing phone number");
	else
		(*acu->acu_abort)();
	return (tried ? "call failed" : "missing phone number");
}

disconnect()
{
	if (!conflag)
		return;
	logent(value(HOST), "", acu->acu_name, "call terminated");
	if (boolean(value(VERBOSE)))
		printf("\r\ndisconnecting...");
	(*acu->acu_disconnect)();
}

static int
acuabort(s)
{
	signal(s, SIG_IGN);
	longjmp(jmpbuf, 1);
}

static acu_t *
acutype(s)
	register char *s;
{
	register acu_t *p;
	extern acu_t acutable[];

	for (p = acutable; p->acu_name != '\0'; p++)
		if (!strcmp(s, p->acu_name))
			return (p);
	return (NOACU);
}
