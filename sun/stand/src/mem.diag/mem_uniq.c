static char	sccsid[] = "@(#)mem_uniq.c 1.1 9/25/86 Copyright Sun Microsystems";

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

extern int	datamode, errmessmode, toterr, buserrcount;

int
unique(addr, size)
register u_char	*addr;
register u_long	size;
{
	register u_long	increment, passcnt, i, failindex, fsize;
	register u_char *faddr;
	int		perrs = 0;
	u_long		sval;
	char		c;

	if (errmessmode)
		printf("\nUniqueness Test\n");

	addr = (u_char *)eattoken(addr, addr, addr, 16);
	size = eattoken(size - (u_long)addr, size - (u_long)addr,
			size - (u_long)addr, 16);
	increment = eattoken(1, 1, 1, 16);
	passcnt = eattoken(1, 1, 0xffffffff, 10);

	for (i = 1; i <= passcnt; i++) {
		faddr = addr;
		fsize = size;
		sval = 0;
		switch (datamode) {
			case 0 :	/* byte mode */
				do {
					if (failindex = buunique(faddr, fsize,
							increment, sval)) {
						faddr = addr + size - failindex;
						errhand(testname, faddr,
						  sval = (u_char)((size -
						  failindex + 1) * increment),
						  *(u_char *)(faddr));
						++perrs;
						++faddr;
						fsize = addr + size - faddr;
					}
					if ((c = maygetchar()) != -1) {
						printf("exiting %s", testname);
						return('q');
					}
				} while (failindex && fsize);
				break;
			case 1 :	/* word mode */
				do {
					if (failindex = wuunique(faddr, fsize,
							increment, sval)) {
						faddr = addr+size-2*failindex;
						errhand(testname, faddr,
						  sval = (u_short)((size / 2 -
						  failindex + 1) * increment),
						  *(u_short *)(faddr));
						++perrs;
						faddr += 2;
						fsize = addr + size - faddr;
					}
					if ((c = maygetchar()) != -1) {
						printf("exiting %s", testname);
						return('q');
					}
				} while (failindex && fsize);
				break;
			case 2 :	/* long mode */
				do {
					if (failindex = luunique(faddr, fsize,
							increment, sval)) {
						faddr = addr+size-4*failindex;
						errhand(testname, faddr,
						  sval - (u_long)((size / 4 -
						  failindex + 1) * increment),
						  *(u_long *)(faddr));
						++perrs;
						faddr += 4;
						fsize = addr + size - faddr;
					}
					if ((c = maygetchar()) != -1) {
						printf("exiting %s", testname);
						return('q');
					}
				} while (failindex && fsize);
				break;
		}
	printf("pass %d of %s : %d errors : %d data errors : %d bus errors\n",
			i, testname, perrs, toterr, buserrcount);
	}

	return(0);
}

