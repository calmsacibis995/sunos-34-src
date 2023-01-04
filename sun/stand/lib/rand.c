#include <sys/types.h>
/*
 *	this code is the random fill/check/o_check stuff.  It uses
 *	the degree 7 RNG that random(3) and ../misc/random use, except
 *	that the random() call is buried in the memory routines to save
 *	subroutine overhead.
 */

#define		TYPE		1		/* x**7 + x**3 + 1 */
#define		DEG		7
#define		SEP		3
#define		END		(state + DEG)

/*
 *	the forward and rear pointers end up in the same place when
 *	startrand is done, so no global pointers exist.
 */

extern u_long	obs_value, exp_value;
static u_long	state[DEG];

#ifdef SIZEOK
static char	sccsid[] = "@(#)rand.c 1.1 86/09/25 SMI";
#endif

static
startrand(seed)
unsigned		seed;
{
	register	i;
	register u_long	*forward = state,
			*rear = state + SEP,
			*endp = state + DEG;

	*state = seed;
	for( i = 1; i < DEG; i++ )  {
		state[i] = 1103515245 * state[i-1] + 12345;
	}
	forward = state + SEP;
	rear = state;
	for( i = 0; i < 10*DEG; i++ ){
		*forward++ += *rear++;
		if (forward == END) {
			forward -= DEG;			/* reset to state */
		} else {
			if (rear == END) {		/* reset to state */
				rear -= DEG;		/* subql is faster */
			}				/* movl #, a[0-7] */
		}
	}
}

brfill(addr, size, seed)
register u_char		*addr;
register u_long		size;
u_long			seed;
{
	register u_long	*rear = state,
			*forward = state + SEP,
			*endp = state +DEG;

	startrand(seed);
	if (size) {
		do {
			*addr++ = (*forward++ += *rear++);
			if (forward == endp) {
				forward -= DEG;		/* reset to state */
			} else {
				if (rear == endp) {
					rear -= DEG;	/* reset to state */
				}
			}
		} while(--size);
	}
}

wrfill(addr, size, seed)
register u_short	*addr;
register u_long		size;
u_long			seed;
{
	register u_long	*rear = state,
			*forward = state + SEP,
			*endp = state +DEG;

	startrand(seed);
	size /= sizeof(u_short);
	if (size) {
		do {
			*addr++ = (*forward++ += *rear++);
			if (forward == endp) {
				forward -= DEG;		/* reset to state */
			} else {
				if (rear == endp) {
					rear -= DEG;	/* reset to state */
				}
			}
		} while(--size);
	}
}

lrfill(addr, size, seed)
register u_long		*addr;
register u_long		size;
u_long			seed;
{
	register u_long	*rear = state,
			*forward = state + SEP,
			*endp = state +DEG;

	startrand(seed);
	size /= sizeof(u_long);
	if (size) {
		do {
			*addr++ = (*forward++ += *rear++);
			if (forward == endp) {
				forward -= DEG;		/* reset to state */
			} else {
				if (rear == endp) {
					rear -= DEG;	/* reset to state */
				}
			}
		} while(--size);
	}
}

brcheck(addr, size, seed)
register u_char		*addr;
register u_long		size;
u_long			seed;
{
	register u_long	*rear = state,
			*forward = state + SEP,
			*endp = state +DEG;

	startrand(seed);
	if (size) {
		do {
			if(*addr++ != ((*forward++ += *rear++) & 0xff)){
				exp_value = 0xff & *(forward - 1);
				return(size);
			}
			if (forward == endp) {
				forward -= DEG;		/* reset to state */
			} else {
				if (rear == endp) {
					rear -= DEG;	/* reset to state */
				}
			}
		} while(--size);
	}
	return(size);
}

wrcheck(addr, size, seed)
register u_short	*addr;
register u_long		size;
u_long			seed;
{
	register u_long	*rear = state,
			*forward = state + SEP,
			*endp = state +DEG;

	startrand(seed);
	size /= sizeof(u_short);
	if (size) {
		do {
			if(*addr++ != ((*forward++ += *rear++) & 0xffff)){
				exp_value = 0xffff & *(forward - 1);
				return(size);
			}
			if (forward == endp) {
				forward -= DEG;		/* reset to state */
			} else {
				if (rear == endp) {
					rear -= DEG;	/* reset to state */
				}
			}
		} while(--size);
	}
	return(size);
}

lrcheck(addr, size, seed)
register u_long		*addr;
register u_long		size;
u_long			seed;
{
	register u_long	*rear = state,
			*forward = state + SEP,
			*endp = state +DEG;

	startrand(seed);
	size /= sizeof(u_long);
	if (size) {
		do {
			if(*addr++ != (*forward++ += *rear++)){
				exp_value = *(forward - 1);
				return(size);
			}
			if (forward == endp) {
				forward -= DEG;		/* reset to state */
			} else {
				if (rear == endp) {
					rear -= DEG;	/* reset to state */
				}
			}
		} while(--size);
	}
	return(size);
}

o_brcheck(addr, size, seed)
register u_char		*addr;
register u_long		size;
u_long			seed;
{
	register u_long	*rear = state,
			*forward = state + SEP,
			*endp = state +DEG;
	register u_char	obs;

	startrand(seed);
	if (size) {
		do {
			if((obs = *addr++) !=
				((*forward++ += *rear++) & 0xff)){
				exp_value = 0xff & *(forward - 1);
				obs_value = obs;
				return(size);
			}
			if (forward == endp) {
				forward -= DEG;		/* reset to state */
			} else {
				if (rear == endp) {
					rear -= DEG;	/* reset to state */
				}
			}
		} while(--size);
	}
	return(size);
}

o_wrcheck(addr, size, seed)
register u_short	*addr;
register u_long		size;
u_long			seed;
{
	register u_long	*rear = state,
			*forward = state + SEP,
			*endp = state +DEG;
	register u_short	obs;

	startrand(seed);
	size /= sizeof(u_short);
	if (size) {
		do {
			if((obs = *addr++) !=
				((*forward++ += *rear++) & 0xffff)){
				exp_value = 0xffff & *(forward - 1);
				obs_value = obs;
				return(size);
			}
			if (forward == endp) {
				forward -= DEG;		/* reset to state */
			} else {
				if (rear == endp) {
					rear -= DEG;	/* reset to state */
				}
			}
		} while(--size);
	}
	return(size);
}

o_lrcheck(addr, size, seed)
register u_long		*addr;
register u_long		size;
u_long			seed;
{
	register u_long	*rear = state,
			*forward = state + SEP,
			*endp = state +DEG;
	register u_long	obs;

	startrand(seed);
	size /= sizeof(u_long);
	if (size) {
		do {
			if((obs = *addr++) != (*forward++ += *rear++)){
				exp_value = *(forward - 1);
				obs_value = obs;
				return(size);
			}
			if (forward == endp) {
				forward -= DEG;		/* reset to state */
			} else {
				if (rear == endp) {
					rear -= DEG;	/* reset to state */
				}
			}
		} while(--size);
	}
	return(size);
}
