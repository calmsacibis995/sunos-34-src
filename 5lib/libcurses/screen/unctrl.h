/*	@(#)unctrl.h 1.1 86/09/24 SMI; from S5R2 1.1	*/

/*
 * unctrl.h
 *
 * 1/26/81 (Berkeley) @(#)unctrl.h	1.1
 */

#ifndef unctrl
extern char	*_unctrl[];

# define	unctrl(ch)	(_unctrl[(unsigned) ch])
#endif
