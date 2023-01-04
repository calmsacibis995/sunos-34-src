#ifndef lint
static char sccsid[]= "@(#)dosys.c 1.3 87/04/17 Copyr 1986 Sun Micro";
#endif lint

/*
 *	Copyright (c) 1986 by Sun Microsystems, Inc	[Remotely from S5R2]
 */

#include "defs.h"
#include <sys/wait.h>
#include <sys/signal.h>
#include <errno.h>
#include <ctype.h>

/*
 *	await() wait for one child process and analyzes
 *	the returned status when the child process terminates.
 */
Boolean
await(ignore_error, silent_error)
	register Boolean	ignore_error;
	register Boolean	silent_error;
{
	union wait		status;
	register int		pid;

	enable_interrupt(handle_interrupt);
	while ((pid= wait(&status)) != process_running)
		if (pid == -1)
			fatal("wait() failed: %s", errmsg(errno));
	process_running= -1;
	if (status.w_status == 0)
		return(succeeded);

	/* If the child returned an error we now try to print a nice message */
	/* about that */
	if (is_false(silent_error)) {
		(void)fflush(stdout);
		if (status.w_T.w_Retcode != 0)
			(void)fprintf(stderr, "*** Error code %d", status.w_T.w_Retcode);
		else {
			if (status.w_T.w_Termsig > NSIG)
				(void)fprintf(stderr, "*** Signal %d", status.w_T.w_Termsig);
			else
				(void)fprintf(stderr, "*** %s", sys_siglist[status.w_T.w_Termsig]);
			if (status.w_T.w_Coredump)
				(void)fprintf(stderr, " - core dumped");
		};
		if (is_true(ignore_error))
			(void)fprintf(stderr, " (ignored)");
		(void)fprintf(stderr, "\n");
		(void)fflush(stderr);
	};
	return(failed);
}

/*
 *	doshell() is used to run command lines that include shell meta-characters
 *	The make macro SHELL is supposed to contain a path to the shell
 */
static int
doshell(command, ignore_error)
	char			*command;
	register Boolean	ignore_error;
{
	register char		*shellname;
	char			*argv[4];
	register Name		shell= getvar(cached_names.shell);
	register int		waitpid;

	if ((shellname= rindex(shell->string, SLASH)) == NULL)
		shellname= shell->string;
	else
		shellname++;
	argv[0]= shellname;
	argv[1]= is_true(ignore_error) ? "-c" : "-ce";
	argv[2]= command;
	argv[3]= NULL;
	(void)fflush(stdout);
	if ((waitpid= vfork()) == 0) {
		enable_interrupt((int (*) ()) SIG_DFL);
		shell->string[shell->hash.length]= NUL;
		(void)execve(shell->string, argv, environ);
		fatal("Could not load Shell from `%s': %s", shell->string,
			errmsg(errno));
	};
	return(waitpid);
}

/*
 *	exec_vp(name, argv, envp)	(like execve, but does path search)
 *	This starts command when make invokes it directly (without a shell)
 */
static Boolean
exec_vp(name, argv, envp)
	register char		*name;
	register char		**argv;
	char			*envp[NCARGS];
{
	register int		etxtbsy= 1;

try_again:
	(void)execve_vroot(name, argv, envp, vroot_path, VROOT_DEFAULT);
	switch (errno) {
	    case ENOEXEC:
		/* That failed. Let the shell handle it */
		*argv= name;
		*--argv= "sh";
		(void)execve_vroot(getvar(cached_names.shell)->string,
				   argv,
				   envp,
				   vroot_path,
				   VROOT_DEFAULT);
		return(failed);
	    case ETXTBSY:
		/* The program is busy (debugged?). Wait and then try again */
		if (++etxtbsy > 5)
			return(failed);
		(void)sleep((unsigned) etxtbsy);
		goto try_again;
	    case EACCES:
	    case ENOMEM:
	    case E2BIG:
		return(failed);
	};

	return(failed);
}

#define	MAXARGV	500

/*
 *	doexec() will scan an argument string and split it into words
 *	thus building an argument list that can be passed to exec_ve()
 */
static int
doexec(command)
	register char		*command;
{
	register char		*t;
	register char		**p;
	char			*argv[MAXARGV];
	register int		waitpid;

	p= &argv[1];			 /* reserve argv[0] for sh in case of
					  * exec_vp failure */
	/* Build list of argument words */
	for (t= command; *t;) {
		if (p >= &argv[MAXARGV])
			fatal("Command `%s' has more than %d arguments",
				command, MAXARGV);
		*p++= t;
		while (!isspace(*t) && (*t != NUL))
			t++;
		if (*t)
			for (*t++= NUL; isspace(*t); t++);
	};
	*p= NULL;

	/* Then exec the command with that argument list */
	(void)fflush(stdout);
	if ((waitpid= vfork()) == 0) {
		enable_interrupt((int (*) ()) SIG_DFL);
		(void)exec_vp(command, &argv[1], environ);
		fatal("Cannot load command `%s': %s", command, errmsg(errno));
	};
	return(waitpid);
}

/*
 *	Check if arg string contains meta chars and dispatch to the proper routine
 */
Boolean
dosys(command, ignore_error, call_make, silent_error)
	register Name		command;
	register Boolean	ignore_error;
	register Boolean	call_make;
	Boolean			silent_error;
{
	register char		*p= command->string;
	register char		*q;
	register int		length= command->hash.length;

	/* Strip spaces from head of command string */
	while (isspace(*p)) {
		p++, length--;
	};
	if (*p == NUL) {
		return(failed);
	};
	/* If we are faking it we just return */
	if (is_true(flag.do_not_exec_rule) &&
	    is_true(working_on_targets) &&
	    is_false(call_make)) {
		return(succeeded);
	};

	/* Copy string to make it OK to write it */
	q= alloca(length + 1);
	(void)strcpy(q, p);
	/* Run command directly if it contains no shell meta chars else run it */
	/* using the shell */
	process_running= is_true(command->meta) ? doshell(q, ignore_error) : doexec(q);
	return(await(ignore_error, silent_error));
}
