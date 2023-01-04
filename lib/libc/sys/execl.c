/*	@(#)execl.c 1.1 86/09/24 SMI; from S5R2 1.1	*/

/*
 *	execl(name, arg0, arg1, ..., argn, (char *)0)
 *	environment automatically passed.
 */

execl(name, args)
char *name, *args;
{
	extern char **environ;

	return (execve(name, &args, environ));
}
