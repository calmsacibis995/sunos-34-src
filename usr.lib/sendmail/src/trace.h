/*
**  Trace Package.
**
**	Version:
**	@(#)trace.h 1.1 86/09/25 SMI; from UCB 4.2
*/

typedef u_char	*TRACEV;

extern TRACEV	tTvect;			/* current trace vector */

# ifndef tTVECT
# define tTVECT		tTvect
# endif tTVECT

# define tTf(flag, level)	(tTVECT[flag] >= level)
# define tTlevel(flag)		(tTVECT[flag])
