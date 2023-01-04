static char	sccsid[] = "@(#)strcat.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "strcat" (string concatenate) appends string 't' to the end of
 * string 's'.  Notice that NO checks are made to determine whether or not
 * there is enough room in string 's' to append string 't'.
 */
strcat(s, t)
  char *s;
  char *t;
{

  while(*s != '\0') { /* find end of 's' */
    s++;
  }
  while((*s++ = *t++) != '\0') {
    ;
  }
}


