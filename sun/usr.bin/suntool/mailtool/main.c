#ifndef lint
static	char sccsid[] = "@(#)main.c 1.7 87/01/07 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool
 */

#include <stdio.h>
#include <sys/syscall.h>
#include <sys/file.h>
#include "glob.h"

#ifdef STANDALONE
#define EXIT(n)		exit(n)
#else
#define EXIT(n)		return(n)
#endif

char    *mktemp(), *strcpy(), *sprintf();

int	mt_aborting;		/* aborting, don't save messages */
char	*mt_cmdname;		/* our name */
int	mt_mailclient;		/* client handle */

char	mt_hdrfile[32];
char	mt_msgfile[32];
char	mt_replyfile[32];
char	mt_printfile[32];
char	mt_dummybox[32];

#ifdef STANDALONE
main(argc, argv)
#else
mailtool_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	char **tool_attrs = NULL;
	int i;
	char *iv = NULL;
	int expert = 0;

	mt_init_mail_storage();
	mt_init_tool_storage();
	for (i = getdtablesize(); i > 2; i--)
		(void)close(i);
	mt_cmdname = argv[0];
	argc--;
	argv++;
	while (argc > 0) {
		if (argv[0][0] == '-') {
			if (argv[0][2] != '\0')
				goto toolarg;
			switch (argv[0][1]) {
			case 'x':
				expert++;
				break;
			case 'i':
				if (argc < 2)
					usage(mt_cmdname);
				iv = argv[1];
				argc--;
				argv++;
				break;
			default:
			toolarg:
				/*
				 * Pick up generic tool arguments.
				 */
				if ((i = tool_parse_one(argc, argv,
				    &tool_attrs, mt_cmdname)) == -1) {
					(void)tool_usage(mt_cmdname);
					EXIT(1);
				} else if (i == 0)
					usage(mt_cmdname);
				argc -= i;
				argv += i;
				continue;
			}
		} else {
			usage(mt_cmdname);
		}
		argc--;
		argv++;
	}

	/*
	 * Determine user's mailbox.
	 */
	if (getenv("MAIL"))
		(void)strcpy(mt_mailbox, getenv("MAIL"));
	else
		(void)sprintf(mt_mailbox, "/usr/spool/mail/%s", getenv("USER"));/* XXX */

	if (mt_init_tmpfiles() < 0)
		return;
	mt_start_mail();
	mt_get_vars();
	if (iv)
		mt_assign("interval", iv);
	if (expert)
		mt_assign("expert", "");
		
	if (!mt_mail_seln_exists())
		EXIT(1);
		
	mt_start_tool(tool_attrs);
	mt_done(0);
	mt_release_mail_storage();
	
	EXIT(0);
}

static
usage(name)
	char *name;
{
	
	(void)fprintf(stderr, "Usage: %s [-x] [-i interval] [tool args]\n", name);
	exit(1);
}

mt_init_tmpfiles()
{

	(void)strcpy(mt_hdrfile, "/tmp/MThXXXXXX");
	(void)strcpy(mt_msgfile, "/tmp/MTmXXXXXX");
	(void)strcpy(mt_replyfile, "/tmp/MTrXXXXXX");
	(void)strcpy(mt_printfile, "/tmp/MTpXXXXXX");
	(void)strcpy(mt_dummybox, "/tmp/MTdXXXXXX");
	(void) mktemp(mt_hdrfile);
	(void) mktemp(mt_msgfile);
	(void) mktemp(mt_replyfile);
	(void) mktemp(mt_printfile);
	(void) mktemp(mt_dummybox);
	return(0);
}

mt_done(i)
	int i;
{

	(void) unlink(mt_hdrfile);
	(void) unlink(mt_msgfile);
	(void) unlink(mt_replyfile);
	(void) unlink(mt_printfile);
	(void) unlink(mt_dummybox);
	exit(i);
}
