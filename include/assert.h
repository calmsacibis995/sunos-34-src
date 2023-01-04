/*	@(#)assert.h 1.1 86/09/24 SMI; from UCB 4.1 83/05/03	*/

# ifndef NDEBUG
# define _assert(ex) {if (!(ex)){fprintf(stderr,"Assertion failed: file %s, line %d\n", __FILE__, __LINE__);exit(1);}}
# define assert(ex) {if (!(ex)){fprintf(stderr,"Assertion failed: file %s, line %d\n", __FILE__, __LINE__);exit(1);}}
# else
# define _assert(ex) ;
# define assert(ex) ;
# endif
