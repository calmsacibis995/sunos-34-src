/*	@(#)lastlog.h 1.1 86/09/24 SMI; from UCB 4.2 83/05/22	*/

struct lastlog {
	time_t	ll_time;
	char	ll_line[8];
	char	ll_host[16];		/* same as in utmp */
};
