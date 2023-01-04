static char	sccsid[] = "@(#)mem_const.c 1.1 9/25/86 Copyright Sun Microsystems";

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

extern int	datamode, errmessmode, toterr, buserrcount;

int
const(addr, size)
register u_char	*addr;
register u_long	size;
{
	register u_long	pattern, passcnt, i, failindex, fsize;
	register u_char	*faddr;
	int		perrs = 0;
	char		c;

	if (errmessmode)
		printf("\nConstant Pattern Test\n");

	addr = (u_char *)eattoken(addr, addr, addr, 16);
	size = eattoken(size - (u_long)addr, size - (u_long)addr,
			size - (u_long)addr, 16);
	pattern = eattoken(0x55555555, 0x55555555, 0x55555555, 16);
	passcnt = eattoken(1, 1, 0xffffffff, 10);

	for (i = 1 ; i <= passcnt ; i++) {
		faddr = addr;
		fsize = size;
		switch (datamode) {
			case 0 :	/* byte mode */
				bfill(addr, size, pattern);
				do {
					if (failindex = bcheck(faddr, fsize,
								pattern)) {
						faddr = faddr+fsize-failindex;
						errhand(testname, faddr,
							(u_char)pattern,
						  	*(u_char *)(faddr));
						++perrs;
						faddr += 1;
						fsize = addr + size - faddr;
					}
					if ((c = maygetchar()) != -1) {
					    printf("exiting %s\n", testname);
					    return('q');
					}
				} while (failindex && fsize);
				break;
			case 1 :	/* word mode */
				wfill(addr, size, pattern);
				do {
					if (failindex = wcheck(faddr, fsize,
								pattern)) {
						faddr = faddr+fsize-2*failindex;
						errhand(testname, faddr,
							(u_short)pattern,
						  	*(u_short *)(faddr));
						++perrs;
						faddr += 2;
						fsize = addr + size - faddr;
					}
					if ((c = maygetchar()) != -1) {
					    printf("exiting %s\n", testname);
					    return('q');
					}
				} while (failindex && fsize);
				break;
			case 2 :	/* long mode */
				lfill(addr, size, pattern);
				do {
					if (failindex = lcheck(faddr, fsize,
								pattern)) {
						faddr = faddr+fsize-4*failindex;
						errhand(testname, faddr,
							(u_long)pattern,
						  	*(u_long *)(faddr));
						++perrs;
						faddr += 4;
						fsize = addr + size - faddr;
					}
					if ((c = maygetchar()) != -1) {
					    printf("exiting %s\n", testname);
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

