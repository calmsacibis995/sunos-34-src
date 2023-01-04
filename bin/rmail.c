#ifndef lint
static	char sccsid[] = "@(#)rmail.c 1.1 86/09/24 SMI"; /* from UCB 4.5 8/18/84 */
#endif

/*
 * rmail -- UUCP mail server.
 *
 * This program reads the >From ... remote from ... lines that
 * UUCP is so fond of and turns them into something reasonable.
 * It calls sendmail giving it a -f option built from these lines.
 */
#include <stdio.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef char	bool;
#define TRUE	1
#define FALSE	0

extern FILE	*popen();
extern char	*index();
extern char	*rindex();

bool	Debug;

# define MAILER	"/usr/lib/sendmail"

main(argc, argv)
	char **argv;
{
	FILE *out;	/* output to sendmail */
	char lbuf[512];	/* one line of the message */
	char from[512];	/* accumulated path of sender */
	char ufrom[512]; /* user on remote system */
	char sys[512];	/* a system in path */
	char junk[512];	/* scratchpad */
	char *args[100];/* arguments to mailer command */
	register char *cp;
	register char *uf;	/* ptr into ufrom */
	int i;
	long position;
	struct stat sbuf;
	

# ifdef DEBUG
	if (argc > 1 && strcmp(argv[1], "-T") == 0)
	{
		Debug = TRUE;
		argc--;
		argv++;
	}
# endif DEBUG

	if (argc < 2)
	{
		fprintf(stderr, "Usage: rmail user ...\n");
		exit(EX_USAGE);
	}

	(void) strcpy(from, "");
	(void) strcpy(ufrom, "/dev/null");

	for (position=0;;position=ftell(stdin))
	{
		(void) fgets(lbuf, sizeof lbuf, stdin);
		if (strncmp(lbuf, "From ", 5) != 0 && strncmp(lbuf, ">From ", 6) != 0)
			break;
		(void) sscanf(lbuf, "%s %s", junk, ufrom);
		cp = lbuf;
		uf = ufrom;
		for (;;)
		{
			cp = index(cp+1, 'r');
			if (cp == NULL)
			{
				register char *p = rindex(uf, '!');

				if (p != NULL)
				{
					*p = '\0';
					(void) strcpy(sys, uf);
					uf = p + 1;
					break;
				}
				(void) strcpy(sys, "");
				break;	/* no "remote from" found */
			}
#ifdef DEBUG
			if (Debug)
				printf("cp='%s'\n", cp);
#endif
			if (strncmp(cp, "remote from ", 12)==0)
				break;
		}
		if (cp != NULL)
			(void) sscanf(cp, "remote from %s", sys);
		if (sys[0])
		{
			(void) strcat(from, sys);
			(void) strcat(from, "!");
		}
#ifdef DEBUG
		if (Debug)
			printf("ufrom='%s', sys='%s', from now '%s'\n", uf, sys, from);
#endif
	}
	(void) strcat(from, uf);
	(void) fstat(0,&sbuf);
	(void) lseek(0,position,0);

	  /*
	   * Now we rebuild the argument list and chain to sendmail.
	   * Note that the above lseek might fail on irregular files,
	   * but we check for that case below.
	   */
	args[0] = MAILER;
	args[1] = "-ee";
	(void) sprintf(junk, "-f%s", from);
	args[2] = junk;
	if (from[0] == '\0') 
		i = 2;
	else
		i = 3;
	for (;*++argv != NULL;i++)
	{
		args[i] = *argv;
	}
	args[i] = NULL;
#ifdef DEBUG
	if (Debug)
	{
		printf("Command:");
		for (i=0;args[i];i++)
			printf(" %s",args[i]);
		printf("\n");
	}
#endif
	if ( (sbuf.st_mode & S_IFMT) != S_IFREG)
	{
	    /*
	     * If we were not called with standard input on a regular
	     * file, then we have to fork another process to send the
	     * first line down the pipe.
	     */
	    int pipefd[2];
# ifdef DEBUG
	    if (Debug) printf("Not a regular file!\n");
# endif DEBUG
	    if (pipe(pipefd) < 0) exit(EX_OSERR);
	    if (fork()==0)
	    {
	      /*
	       * Child: send the message down the pipe.
	       */
		FILE *out;
	       
		out = fdopen(pipefd[1],"w");
		close(pipefd[0]);
		fputs(lbuf, out);
		while (fgets(lbuf, sizeof lbuf, stdin))
			fputs(lbuf, out);
		fclose(out);
		exit(EX_OK);
	    }
	      /*
	       * Parent: call sendmail with pipe as standard input
	       */
	    close(pipefd[1]);
	    dup2(pipefd[0],0);
	}
	execv(MAILER,args);
	printf("Exec of %s failed!\n", MAILER);
	exit(EX_OSERR);
}
