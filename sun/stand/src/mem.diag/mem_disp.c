
static char	sccsid[] = "@(#)mem_disp.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * memory test subroutine
 *
 *		display a block of memory
 */

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

/*
 */

int
memdisp(addr, size)
register u_char		*addr;
register u_long		size;
{
	char		c;

	addr = (u_char *)eattoken(addr, addr, addr, 16);
	size = eattoken(size - (u_long)addr, size - (u_long)addr,
			size - (u_long)addr, 16);

	printf("%x:  ", addr);
	do {
		if (*addr < 0x10) 
			printf(" 0%x", *addr++);
		else
			printf(" %x", *addr++);

		if (((u_long)addr % 16) == 0)
			printf("\n%x:  ", addr);
		else if (((u_long)addr % 4) == 0)
			printf("  ");

		if ((c = maygetchar()) != -1) {
			switch (c) {
				case '\023' :		/* pause on ^s */
					c = getchar();
					break;
				default :
					return('q');
			}
		}

	} while (--size > 0);

	return(0);

}

