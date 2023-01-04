/*	@(#)timeb.h 1.1 86/09/25 SMI; from UCB 4.2 81/02/19	*/

/*
 * Structure returned by ftime system call
 */
struct timeb
{
	time_t	time;
	unsigned short millitm;
	short	timezone;
	short	dstflag;
};
