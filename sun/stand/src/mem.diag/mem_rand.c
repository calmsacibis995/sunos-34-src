static char	sccsid[] = "@(#)mem_rand.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * memory test subroutine
 */

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

#define	baddr	((u_char *)taddr)
#define	waddr	((u_short *)taddr)
#define	laddr	((u_long *)taddr)

extern int	datamode, errmessmode, toterr, buserrcount;
static long	dummy[32] = {3};

mrandom(addr,size)	/* random data test */
register u_char	*addr;
register u_long	size;
{
	register u_long		pattern, passcnt, i;
	int			seed;
	register		count;
	register		taddr;

        register u_long		exp, obs;
	int			perrs = 0;

	char			*ind = "-\\|/";
	int			indp = 0;

	char			c;

	if (errmessmode)
		printf("\nRandom Pattern Test\n");

	addr = (u_char *)eattoken(addr, addr, addr, 16);
	size = eattoken(size - (u_long)addr, size - (u_long)addr,
			size - (u_long)addr, 16);
	seed = eattoken(7, 7, 7, 10);
	passcnt = eattoken(1, 1, 0xffffffff, 10);

	printf("%c\b", ind[indp++ % 4]);
	for (i = 1 ; i <= passcnt ; i++) {
		initstate(seed, dummy, 128);	/* set up seeded rand seq */
		switch (datamode) {		/* put rands into memory */
			case 0 :		/* byte mode */
				baddr = (u_char *)addr;
				count = size/sizeof(u_char);
				do {
					*baddr++ = random();
					if ((count & 0x7fff) == 0) {
						printf("%c\b", ind[indp++ % 4]);
						if ((c = maygetchar()) != -1) {
						    printf("exiting %s\n",
							 	testname);
						    return('q');
						}
					}
				} while(--count);
				break;
			case 1 :		/* word mode */
				waddr = (u_short *)addr;
				count = size/sizeof(u_short);
				do {
					*waddr++ = random();
					if ((count & 0x7fff) == 0) {
						printf("%c\b", ind[indp++ % 4]);
						if ((c = maygetchar()) != -1) {
						    printf("exiting %s\n",
							 	testname);
						    return('q');
						}
					}
				} while(--count);
				break;
			case 2 :		/* long mode */
				laddr = (u_long *)addr;
				count = size/sizeof(u_long);
				do {
					*laddr++ = random();
					if ((count & 0x7fff) == 0) {
						printf("%c\b", ind[indp++ % 4]);
						if ((c = maygetchar()) != -1) {
						    printf("exiting %s\n",
							 	testname);
						    return('q');
						}
					}
				} while(--count);
				break;
		}

		initstate(seed, dummy, 128);	/* set up seeded rand seq */
		switch (datamode) {		/* read rands from memory */
			case 0 :		/* byte mode */
				baddr = (u_char *)addr;
				count = size/sizeof(u_char);
				do {
					if((obs = *baddr++) !=
						(exp = (u_char)random())) {
						errhand(testname,
							baddr - 1, exp, obs);
						++perrs;
					}
					if ((count & 0x7fff) == 0) {
						printf("%c\b", ind[indp++ % 4]);
						if ((c = maygetchar()) != -1) {
						    printf("exiting %s\n",
							 	testname);
						    return('q');
						}
					}
				} while(--count);
				break;
			case 1 :		/* word mode */
				waddr = (u_short *)addr;
				count = size/sizeof(u_short);
				do {
					if((obs = *waddr++) !=
						(exp = (u_short)random())) {
						errhand(testname,
							waddr - 1, exp, obs);
						++perrs;
					}
					if ((count & 0x7fff) == 0) {
						printf("%c\b", ind[indp++ % 4]);
						if ((c = maygetchar()) != -1) {
						    printf("exiting %s\n",
							 	testname);
						    return('q');
						}
					}
				} while(--count);
				break;
			case 2 :		/* long mode */
				laddr = (u_long *)addr;
				count = size/sizeof(u_long);
				do {
					if((obs = *laddr++) !=
						(exp = (u_long)random())) {
						errhand(testname,
							laddr - 1, exp, obs);
						++perrs;
					}
					if ((count & 0x7fff) == 0) {
						printf("%c\b", ind[indp++ % 4]);
						if ((c = maygetchar()) != -1) {
						    printf("exiting %s\n",
							 	testname);
						    return('q');
						}
					}
				} while(--count);
				break;
		}
	printf("pass %d of %s : %d errors : %d data errors : %d bus errors\n",
			i, testname, perrs, toterr, buserrcount);
	}

	return(0);
}

char _iob;

fprintf() { ; }

