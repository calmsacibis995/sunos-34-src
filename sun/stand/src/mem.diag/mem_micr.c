static char	sccsid[] = "@(#)mem_micr.c1.1 9/25/86 Copyright Sun Microsystems";

/*
	Implement another type of diagonal pattern test, to better (ie
	quickly) isolate pattern sensitive ram parts.

	Gale Snow	Wed Feb 11 11:15:00 PDT 1985
*/

#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <token.h>

extern int	errmessmode, toterr, buserrcount;

int
micronpat(addr, size)
register u_char	*addr;
register		size;
{
	register u_long		ras, cas, bank, diagp;
	int			h, j, k, m;
	register u_char		*baddr;
	u_short			value, pat, dpat;
	u_long			passcnt, i;
	int			perrs = 0;
	char			*ind = "-\\|/";
	int			indp = 0;
	char			c;

	if (errmessmode)
		printf("\nMicron's Diagonal Test\n");

	addr = (u_char *)eattoken(addr, addr, addr, 16);
	size = eattoken(size - (u_long)addr, size - (u_long)addr,
			size - (u_long)addr, 16);
	passcnt = eattoken(1, 1, 0xffffffff, 10);

	if ((((u_long)addr % 0x40000) != 0) || ((size % 0x40000) != 0)) {
    printf("\n>> addr and size parameters must be a multiple of 0x40000 <<\n");
	  return(1);
	}
	/*	start addr as well as size is assumed to be
		a multiple of 0x40000 - we're dealing with bank pairs here
	 */

	for (i = 1; i <= passcnt; i++) {

	  for (baddr = addr; baddr < addr + size; baddr += 0x40000) {

	    for (h = 0, pat = 0; h < 2; h +=1, pat = ~pat) {

	      /* initialize the bank pair */
	      wfill(baddr, 0x40000, pat);

	      for (k = 0, bank = (int)baddr;
	           k < 2; k += 1, bank += 0x200) {

	        for (diagp = 0; diagp < 0x100; diagp += 1) {

		  /* print an indication that something's happening
 		   */
		  if (errmessmode) printf("%c\b", ind[indp++ %4]);

		  for (m = 0, dpat = pat; m < 2; m += 1, dpat = ~dpat) {

		    /* read the diagonal first */
		    for (j = 0, ras = (diagp * 2), cas = 0;
		         j < 0x80;
		         j += 1, ras = (ras + 4) % 0x200, cas += 0x400) {

			value = *(u_short *)(bank + cas + ras);
			if (value != dpat) {
			  errhand(testname, bank + cas + ras, dpat, value);
			  ++perrs;
			}

			value = *(u_short *)(bank + (cas + 0x20000) +
							((ras + 2) % 0x200));
			if (value != dpat) {
			  errhand(testname, bank + (cas + 0x20000) +
				 	((ras + 2) % 0x200), dpat, value);
			  ++perrs;
			}
		    }

		    /* then write it */
		    for (j = 0, ras = (diagp * 2), cas = 0;
		         j < 0x80;
		         j += 1, ras = (ras + 4) % 0x200, cas += 0x400) {

			*(u_short *)(bank + cas + ras) = ~dpat;
			*(u_short *)(bank + (cas + 0x20000) +
						((ras + 2) % 0x200)) = ~dpat;
		    }


			/* >> POSSIBLY DO REFRESH TYPE TEST HERE << */

		  }

		  /* see if char has been typed to quit */
		  if ((c = maygetchar()) != -1) {
			printf("exiting %s\n", testname);
			return('q');
		  }

		}
	      }
	    }


	    for (h = 0, pat = 0; h < 2; h +=1, pat = ~pat) {

	      /* initialize the bank pair */
	      wfill(baddr, 0x40000, pat);

	      for (k = 0, bank = (int)baddr;
	           k < 2; k += 1, bank += 0x200) {

	        for (diagp = 0x100; diagp > 0; diagp -= 1) {

		  /* print an indication that something's happening
 		   */
		  if (errmessmode) printf("%c\b", ind[indp++ %4]);

		  for (m = 0, dpat = pat; m < 2; m += 1, dpat = ~dpat) {

		    /* read the diagonal first */
		    for (j = 0, ras = ((diagp - 1) * 2), cas = 0;
		         j < 0x80;
		         j += 1, ras = (ras - 4) % 0x200, cas += 0x400) {

			value = *(u_short *)(bank + cas + ras);
			if (value != dpat) {
			  errhand(testname, bank + cas + ras, dpat, value);
			  ++perrs;
			}

			value = *(u_short *)(bank + (cas + 0x20000) +
							((ras - 2) % 0x200));
			if (value != dpat) {
			  errhand(testname, bank + (cas + 0x20000) +
				 	((ras - 2) % 0x200), dpat, value);
			  ++perrs;
			}
		    }

		    /* then write it */
		    for (j = 0, ras = ((diagp - 1) * 2), cas = 0;
		         j < 0x80;
		         j += 1, ras = (ras - 4) % 0x200, cas += 0x400) {

			*(u_short *)(bank + cas + ras) = ~dpat;
			*(u_short *)(bank + (cas + 0x20000) +
						((ras - 2) % 0x200)) = ~dpat;
		    }


			/* >> POSSIBLY DO REFRESH TYPE TEST HERE << */

		  }

		  /* see if char has been typed to quit */
		  if ((c = maygetchar()) != -1) {
			printf("exiting %s\n", testname);
			return('q');
		  }

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

