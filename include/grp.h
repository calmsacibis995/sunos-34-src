/*	@(#)grp.h 1.1 86/09/24 SMI; from UCB 4.1 83/05/03	*/

struct	group { /* see getgrent(3) */
	char	*gr_name;
	char	*gr_passwd;
	int	gr_gid;
	char	**gr_mem;
};

struct group *getgrent(), *getgrgid(), *getgrnam();
