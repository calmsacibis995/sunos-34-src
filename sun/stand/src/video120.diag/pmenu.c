static char	sccsid[] = "@(#)pmenu.c 1.1 9/25/86 Copyright Sun Microsystems";

/*
 * Don't change the order of the include files!
 */
#include <sys/types.h>
#include <machdep.h>
#include <sunromvec.h>
#include <s2addrs.h>
#include <token.h>
#include "video120.h"


/*
 * Function "pmenu" (print menu) displays the "menu" pointed to by "mptr".
 */
pmenu() 
{
  extern struct  menu *mptr; /* defined in                        */
  extern int     nbr;        /* .../stand/src/video120.diag/mm.c */

  struct menu *xptr=mptr;
  int    i=0;

  for(i=0; i < nbr; i++, xptr++) {
    printf("%c  -  %s\n", xptr->t_char, xptr->t_name);
  }
}
