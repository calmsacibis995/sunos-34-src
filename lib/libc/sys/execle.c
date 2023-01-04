/*	@(#)execle.c 1.1 86/09/24 SMI; from S5R2 1.1	*/

/*
 *	execle(file, arg0, arg1, ..., argn, (char *)0, envp)
 */

execle(file, args)
	char	*file;
	char	*args;			/* first arg */
{
	register  char  **p;

	p = &args;
	while(*p++);
	return(execve(file, &args, *p));
}
