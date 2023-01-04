#ifndef lint
static  char sccsid[] = "@(#)m2d_main.c 1.4 87/01/07 Copyr 1985 Sun Micro";
#endif

#include <stdio.h>
#include <strings.h>
#include "m2d_def.h"
#include <sys/stat.h>

void	defaults_remove_private();

extern int	m2d_invoker;
char	*malloc();
char	*sprintf();


/* ARGSUSED */
#ifdef STANDALONE
main(argc, argv)
#else
m2d_main(argc, argv)
#endif
	int argc;
	char **argv;
{
	FILE	*mailrc_fd;
	char	*where, mailrc[2048], backuprc[1024];

	m2d_invoker = MAILRC_TO_DEFAULTS;
	/* open mailrc file */
	if ((where = getenv("MAILRC")) != NULL)
		(void)sprintf(mailrc, "%s", where);
	else
		(void)sprintf(mailrc, "%s%s", getenv("HOME"), "/.mailrc");
        if ((mailrc_fd = fopen(mailrc, "r")) == (FILE *)NULL){
                fputs("defaults to mail conversion: cannot open .mailrc file", stderr);          
                _exit(-1);
        }

        /* backup mailrc file */
        (void)sprintf(backuprc, "%s%s", mailrc, ".OLD");
        if (vfork() == 0) {
                execlp("cp", "cp", mailrc, backuprc, 0);
                _exit(0);
        }

	/* remove all Mail lines from the defaults file */
	defaults_remove_private("/Mail", (int *)NULL);
	
	/* execute the commands in the mailrc file */
	m2d_commands(mailrc_fd);
	/* walk thru the set data and add to defaults */
	m2d_get_sets();
	/* walk thru the alias data and add to defaults */
	(void)m2d_get_aliases();
	m2d_flush_defaults();
	exit(0);
}

