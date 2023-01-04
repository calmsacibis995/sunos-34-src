static char	sccsid[] = "@(#)inrange.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "inrange" (in range) returns TRUE (1) if the value of "var"
 * is greater than or equal to the value of "low" and less than or equal
 * to the value of "high"; otherwise, FALSE (0) is returned.
 */
inrange(var, low, high)
  long var;
  long low;
  long high;
{

  return( (((var >= low) && (var <= high)) ? TRUE : FALSE) );
}




