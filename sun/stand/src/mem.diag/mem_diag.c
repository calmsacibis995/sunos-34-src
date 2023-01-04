static char	sccsid[] = "@(#)mem_diag.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
	Implement the "Moving Diagonal" or "Barber Pole"  memory test.
	Is reputed to be as good as a galpat test, but is only a N^3/2
	type test.

	Joe Murphy	Tue Jun 19 21:53:38 PDT 1984
	Gale Snow	Wed Jul 04 15:03:28 PDT 1984
*/

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

#define	BACKGROUND	0xffff		/* background pattern */

extern int	errmessmode, toterr, buserrcount;

int
diagpat(addr, size)
register u_char	*addr;
register		size;
{
	register int		rowpt, colpt, bank, zappt, chkpt;
	register u_char		*baddr;
	u_short			value;
	u_long			passcnt, i;
	int			perrs = 0;
	char			*ind = "-\\|/";
	int			indp = 0;
	char			c;

	if (errmessmode)
		printf("\nMoving Diagonal Test\n");

	addr = (u_char *)eattoken(addr, addr, addr, 16);
	size = eattoken(size - (u_long)addr, size - (u_long)addr,
			size - (u_long)addr, 16);
	passcnt = eattoken(1, 1, 0xffffffff, 10);

	if ((((u_long)addr % 0x40000) != 0) || ((size % 0x40000) != 0)) {
    printf("\n>> addr and size parameters must be a multiple of 0x40000 <<\n");
	  return(1);
	}

	/*	start addr as well as size is assumed to be
		a multiple of 0x40000
	 */

	/* fill in the backround - ie initialize memory */
	wfill(addr, size, BACKGROUND);

	for (i = 1; i <= passcnt; i++) {

	    for (baddr = addr; baddr < addr + size; baddr += 0x40000) {

		/* 2 banks at a time */
		for (bank = 0; bank <= 0x200; bank += 0x200) {

			/* move the diagonal across the columns */
			for (colpt = 0; colpt < 0x200; colpt += 2) {

				if (errmessmode)
					printf("%c\b", ind[indp++ %4]);

				/* zap everybody on the diagonal */
				for (rowpt = 0; rowpt < 0x40000;
					rowpt += 0x402) {

					*(u_short *)(baddr + bank +
						((rowpt + colpt) & 0x3fdfe)
					 )
						= ~BACKGROUND;
				}


				/* scan down the rows */
				for (rowpt = 0, zappt = 0; rowpt < 0x40000;
					rowpt += 0x400, zappt += 0x402) {

					/* checking each column */
					for (chkpt = rowpt;
						chkpt < (rowpt + 0x200);
						chkpt += 2) {

						/* read memory location once */
						value = *(u_short *)(baddr + bank + chkpt);

						/* should match background */
						if (value != BACKGROUND) {
							/* or it had better
							   be the location
								we zapped */
							if (chkpt !=
							   ((zappt + colpt)
							   & 0x3fdfe)) {
							  errhand(testname,
								baddr + bank +
								chkpt,
								BACKGROUND,
								value);
							  ++perrs;
							} else {
								/* make sure
								   location we
								   zapped
								   stayed
								   zapped */
						if (value != ~BACKGROUND) {
							errhand(testname,
								baddr + bank +
								chkpt,
							(u_short)~BACKGROUND,
								value);
							++perrs;
								}
							}
						}
						if ((c = maygetchar()) != -1) {
						    printf("exiting %s\n",
								testname);
						    return('q');
						}
					}

				}

				/* unzap everybody on the diagonal */
				for (rowpt = 0; rowpt < 0x40000;
					rowpt += 0x402) {

					*(u_short *)(baddr + bank +
						((rowpt + colpt) & 0x3fdfe)
					 )
						= BACKGROUND;
				}

			}
		}
	    }
	if (errmessmode) printf(" \b");
	printf("pass %d of %s : %d errors : %d data errors : %d bus errors\n",
		i, testname, perrs, toterr, buserrcount);
	}
	return(0);
}
