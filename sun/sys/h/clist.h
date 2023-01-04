/*	@(#)clist.h 1.1 86/09/25 SMI; from UCB 4.4 81/03/09	*/

/*
 * Raw structures for the character list routines.
 */
struct cblock {
	struct cblock *c_next;
	char	c_info[CBSIZE];
};
#ifdef KERNEL
struct	cblock *cfree;
int	nclist;
struct	cblock *cfreelist;
int	cfreecount;
#endif
