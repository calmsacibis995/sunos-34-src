static char	sccsid[] = "@(#)mem_opts.c 1.1 9/25/86 Copyright Sun Microsystems";

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

int	datamode;
int	paritymode;
int	errmode;
int	errmessmode;
int	errlimit;

long	toterr;

extern	u_long	*memaddr, memsize;

int
m_option(addr, size)
register u_char	*addr;
register u_long	size;
{
	datamode = eattoken(1,1,1,10);		/* default word mode */
	return(0);
}

int
p_option(addr, size)
register u_char	*addr;
register u_long	size;
{

	paritymode = eattoken(1,1,1,10);	/* default parity on */

	if (paritymode)
		parity(PAR_GEN, PAR_CHECK);
	else
		parity(PAR_GEN, !PAR_CHECK);
	return(0);
}

int
w_option(addr, size)
register u_char	*addr;
register u_long	size;
{

	int tmp = eattoken(0,0,0,10);		/* default wait off */
	if (tmp)
		errmode |= 1;
	else
		errmode &= ~1;
	return(0);
}

int
s_option(addr, size)
register u_char	*addr;
register u_long	size;
{

	int tmp = eattoken(0,0,0,10);		/* default scopeloop off */
	if (tmp)
		errmode |= 2;
	else
		errmode &= ~2;
	return(0);
}

/*
int
l_option(addr, size)
register u_char	*addr;
register u_long	size;
{

	errlimit = eattoken(0xffffffff,0xffffffff,0xffffffff,10);
	return(0);
}
 */

int
e_option(addr, size)
register u_char	*addr;
register u_long	size;
{

	errmessmode = eattoken(1,1,1,10);	/* default all errors */
	return(0);
}

int
set_addr(addr, size)
register u_char	*addr;
register u_long	size;
{
	memaddr = (u_long *)eattoken(addr, addr, addr, 16);
	return(0);
}

int
set_size(addr, size)
register u_char	*addr;
register u_long	size;
{
	memsize = eattoken(size, size, size, 16) + (u_long)memaddr;
	return(0);
}
