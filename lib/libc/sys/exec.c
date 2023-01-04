/*	@(#)exec.c 1.1 86/09/24 SMI	*/

extern char	**environ;

execl(name, argl)
	char	*name;
	char	*argl;
{

	return (execve(name, &argl, environ));
}

execv(name, argv)
	char	*name;
	char	**argv;
{

	return (execve(name, argv, environ));
}

execle(name, argl)
	char	*name;
	char	*argl;
{
	char	**envp = &argl;

	while (*envp++)
		;
	return (execve(name, &argl, *envp));
}
