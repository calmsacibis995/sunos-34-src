/*	@(#)kernel.h 1.1 86/09/25 SMI; from UCB 4.8 83/05/30	*/

/*
 * Global variables for the kernel
 */

long	rmalloc();

/* 1.1 */
char	hostname[32];
int	hostnamelen;
char	domainname[32];
int	domainnamelen;

/* 1.2 */
struct	timeval boottime;
struct	timeval time;
struct	timezone tz;			/* XXX */
int	hz;
int	phz;				/* alternate clock's frequency */
int	tick;
int	lbolt;				/* awoken once a second */
int	realitexpire();

long	avenrun[3];

#ifdef GPROF
extern	int profiling;
extern	char *s_lowpc;
extern	u_long s_textsize;
extern	u_short *kcount;
#endif
