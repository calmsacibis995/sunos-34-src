#include <sys/types.h>

static char	sccsid[] = "@(#)loop.c 1.1 9/25/86 Copyright Sun Micro";

bloop(p, addr, exp, obs)
register char		*p;
register u_char		 *addr, exp, obs;
{
	register u_short	loop;
	register char		c;

	printf(p, addr, exp, obs);
	printf(" .. looping\n");
	while((c = maygetchar()) < 0){
		*addr = exp;
		obs = *addr;
		printf("(0x%x)%c\r", obs, (obs == exp) ? '+' : '-');
	}
	printf("\nstopped looping\n");
	return(c);
}

wloop(p, addr, exp, obs)
register char		*p;
register u_short	 *addr, exp, obs;
{
	register u_short	loop;
	register char		c;

	printf(p, addr, exp, obs);
	printf(" .. looping\n");
	while((c = maygetchar()) < 0){
		*addr = exp;
		obs = *addr;
		printf("(0x%x)%c\r", obs, (obs == exp) ? '+' : '-');
	}
	printf("\nstopped looping\n");
	return(c);
}

lloop(p, addr, exp, obs)
register char		*p;
register u_long		 *addr, exp, obs;
{
	register u_short	loop;
	register char		c;

	printf(p, addr, exp, obs);
	printf(" .. looping\n");
	while((c = maygetchar()) < 0){
		*addr = exp;
		obs = *addr;
		printf("(0x%x)%c\r", obs, (obs == exp) ? '+' : '-');
	}
	printf("\nstopped looping\n");
	return(c);
}
