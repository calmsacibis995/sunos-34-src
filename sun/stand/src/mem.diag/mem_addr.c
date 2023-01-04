static char	sccsid[] = "@(#)mem_addr.c 1.1 9/25/86 Copyright Sun Microsystems";

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

extern int	datamode, errmessmode, toterr, buserrcount;

int
maddress(addr, size)
register u_char	*addr;
register u_long	size;
{
	register u_long	passcnt, i, failindex, fsize;
	register u_char *faddr;
	int		perrs = 0;
	char		c;

	if (errmessmode)
		printf("\nAddress Test\n");

	addr = (u_char *)eattoken(addr, addr, addr, 16);
	size = eattoken(size - (u_long)addr, size - (u_long)addr,
			size - (u_long)addr, 16);
	passcnt = eattoken(1, 1, 0xffffffff, 10);

	for (i = 1; i <= passcnt; i++) {
		faddr = addr;
		fsize = size;
		switch (datamode) {
			case 0 :	/* byte mode */
			    do {
				if (failindex = bmadrtst(faddr, fsize, 0)) {
					faddr = addr + size - failindex;
					errhand(testname, faddr, 
					  (u_char)(faddr), *(u_char *)(faddr));
					++perrs;
					++faddr;
					fsize = addr + size - faddr;
				}
				if ((c = maygetchar()) != -1) {
					printf("exiting %s\n", testname);
					return('q');
				}
			    } while (failindex && fsize);
			    faddr = addr;
			    fsize = size;
			    do {
				if (failindex = bmadrtst(faddr, fsize, -1)) {
					faddr = addr + size - failindex;
					errhand(testname, faddr,
					  (u_char)(faddr) ^ 0xff,
					  *(u_char *)(faddr));
					++perrs;
					++faddr;
					fsize = addr + size - faddr;
				}
				if ((c = maygetchar()) != -1) {
					printf("exiting %s\n", testname);
					return('q');
				}
			    } while (failindex && fsize);
			    break;
			case 1 :	/* word mode */
			    do {
				if (failindex = wmadrtst(faddr, fsize, 0)) {
					faddr = addr + size - 2*failindex;
					errhand(testname, faddr,
					  (u_short)(faddr),
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
			    faddr = addr;
			    fsize = size;
			    do {
				if (failindex = wmadrtst(faddr, fsize, -1)) {
					faddr = addr + size - 2*failindex;
					errhand(testname, faddr,
					  (u_short)(faddr) ^ 0xffff,
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
			    do {
				if (failindex = lmadrtst(faddr, fsize, 0)) {
					faddr = addr + size - 4*failindex;
					errhand(testname, faddr,
					  (u_long)(faddr),
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
			    faddr = addr;
			    fsize = size;
			    do {
				if (failindex = lmadrtst(faddr, fsize, -1)) {
					faddr = addr + size - 4*failindex;
					errhand(testname, faddr,
					  (u_long)(faddr) ^ 0xffffffff,
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

