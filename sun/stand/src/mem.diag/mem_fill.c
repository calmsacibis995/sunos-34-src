static char	sccsid[] = "@(#)mem_fill.c 1.1 9/25/86 Copyright Sun Microsystems";

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

extern int	datamode;

int
fill(addr, size)
register u_char	*addr;
register u_long	size;
{
	register u_long	pattern;

	addr = (u_char *)eattoken(addr, addr, addr, 16);
	size = eattoken(size - (u_long)addr, size - (u_long)addr,
			size - (u_long)addr, 16);
	pattern = eattoken(0xffffffff, 0xffffffff, 0xffffffff, 16);

	switch (datamode) {
		case 0 :	/* byte mode */
			bfill(addr, size, pattern);
			break;
		case 1 :	/* word mode */
			wfill(addr, size, pattern);
			break;
		case 2 :	/* long mode */
			lfill(addr, size, pattern);
			break;
	}

	return(0);
}

