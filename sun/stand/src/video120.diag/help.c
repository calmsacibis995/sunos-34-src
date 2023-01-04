static char	sccsid[] = "@(#)help.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "help" displays more detailed instructions to the user for the
 * "menu" pointed to by "mptr".
 */
help() 
{
  extern struct  menu *mptr; /* defined in                        */
  extern int     nbr;        /* .../stand/src/video120.diag/mm.c */

  struct menu *xptr=mptr;
  int    i=0;

  for (i=0; i < nbr; i++, xptr++) {
    printf("%c  -  %s  %s\n", xptr->t_char, xptr->t_name, xptr->t_help);
  }
  return(0);
}


