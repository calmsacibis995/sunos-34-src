static char	sccsid[] = "@(#)mem_check.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * memory test subroutine
 *
 *		checkerboard test	testcheck(addr,size,pattern)
 */

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

/*
 *	checkertest, writes 2^(incr-1) cells of size/2^(incr-1) with
 *	complementary versions of pattern, as incr goes from 1 to
 *	log2(size) (ASSUMES THAT size IS POWER OF TWO!!!!)
 */

extern int	errmessmode, toterr, buserrcount;

int
checker(addr, size)
register u_char		*addr;
register u_long		size;
{
	register u_long		pattern, i;
	register u_short	value;
	register		incr, count, cellcount, which;
	u_long			passcnt;
	u_short			*test;
	char			*ind = "-\\|/";
	int			perrs = 0, indp = 0;
	char			c;

	if (errmessmode)
		printf("\nChecker Test\n");

	addr = (u_char *)eattoken(addr, addr, addr, 16);
	size = eattoken(size - (u_long)addr, size - (u_long)addr,
			size - (u_long)addr, 16);
	pattern = eattoken(0x5555, 0x5555, 0x5555, 16);
	passcnt = eattoken(1, 1, 0xffffffff, 10);

	if (size & (size-1)) {
	  printf("\n>> size parameter must be a power of 2 <<\n");
	  return(1);
	}

	for (i = 1; i <= passcnt; i++) {
	    for (incr = 1; incr  < size/sizeof(u_short); incr <<= 1) {
/*
 *	    write patterns into memory
 */
		if (errmessmode)
			printf("%c\b", ind[indp++ % 4]);
		if ((c = maygetchar()) != -1) {
			printf("exiting %s\n", testname);
			return('q');
		}
		for(which = 0; which < 2; which++){	/* for data & ~data */
			test = (u_short *) addr + incr*which;
			count = size/(incr*sizeof(u_short)*2);
			do {				/* for each cell */
				cellcount = incr;
				do			/* fill with pattern */
					*test++ = pattern;
				while (--cellcount);
				test += incr;
			} while(--count);
			pattern = ~pattern;		/* invert, & do other */
		}

/*
 *	    read patterns from memory
 */
		if (errmessmode)
			printf("%c\b", ind[indp++ % 4]);
		if ((c = maygetchar()) != -1) {
			printf("exiting %s\n", testname);
			return('q');
		}
		for(which = 0; which < 2; which++){	/* for data & ~data */
			test = (u_short *) addr + incr*which;
			count = size/(incr*sizeof(u_short)*2);
			do {				/* for each cell */
				cellcount = incr;
				do {			/* read pattern back */
				   if ((value = *test++) != (u_short)pattern) {
					errhand(testname,
						test - 1, pattern, value);
					++perrs;
				   }
				}
				while (--cellcount);
				test += incr;
			} while(--count);
			pattern = ~pattern;		/* invert, & do other */
		}
	    }
	    if (errmessmode) printf(" \b");
	printf("pass %d of %s : %d errors : %d data errors : %d bus errors\n",
			i, testname, perrs, toterr, buserrcount);
	}
	return(0);
}

