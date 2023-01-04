static char	sccsid[] = "@(#)crjmp.c 1.1 9/25/86 Copyright Sun Microsystems";

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
 * Function "crjmp" (control register jumper test) performs the "Jumper Test".
 * The "Jumper Test" simply displays the values of bits 08-11 of the control
 * register in hexadecimal, decimal, octal and binary.
 */
crjmp()
{
  extern char errmsg[]; /* defined in .../stand/src/video120.diag/mm.c */
  extern long prterrs;  /* defined in .../stand/src/video120.diag/mm.c */

  long i=0;
  long count=0;
  char *tstname="Jumper Test";
  unsigned short crobs=0;
  unsigned short *craddr=((unsigned short *) CRADDR);

  if (prterrs == TRUE) {
    printf("\n%s\n", tstname);
  }
  /*
   * Read user-specified parameter value and determine whether or not the 
   * value is "legal".
   */
  count=eattoken(0x1, 0x1, INFINITY, 10);
  if (inrange(count, 0x1, INFINITY) == FALSE) {
    strcpy(errmsg, "'count' out of range.");
    return('q');
  }
  /*
   * Perform the "Jumper Test" 'count' times or forever.
   */
  for(i=0; i < count; i++) {
    crobs=(*craddr);
    crobs>>=8;
    crobs&=017;
    printf("\nHexadecimal value of configuration jumpers = 0x%x.\n", crobs);
    printf("Decimal value of configuration jumpers = %d.\n", crobs);
    printf("Octal value of configuration jumpers = 0%o.\n", crobs);
    switch(crobs) {
    case 0:
      printf("Binary value of configuration jumpers = 0000.\n\n", crobs);
      break;
    case 1:
      printf("Binary value of configuration jumpers = 0001.\n\n", crobs);
      break;
    case 2:
      printf("Binary value of configuration jumpers = 0010.\n\n", crobs);
      break;
    case 3:
      printf("Binary value of configuration jumpers = 0011.\n\n", crobs);
      break;
    case 4:
      printf("Binary value of configuration jumpers = 0100.\n\n", crobs);
      break;
    case 5:
      printf("Binary value of configuration jumpers = 0101.\n\n", crobs);
      break;
    case 6:
      printf("Binary value of configuration jumpers = 0110.\n\n", crobs);
      break;
    case 7:
      printf("Binary value of configuration jumpers = 0111.\n\n", crobs);
      break;
    case 8:
      printf("Binary value of configuration jumpers = 1000.\n\n", crobs);
      break;
    case 9:
      printf("Binary value of configuration jumpers = 1001.\n\n", crobs);
      break;
    case 10:
      printf("Binary value of configuration jumpers = 1010.\n\n", crobs);
      break;
    case 11:
      printf("Binary value of configuration jumpers = 1011.\n\n", crobs);
      break;
    case 12:
      printf("Binary value of configuration jumpers = 1100.\n\n", crobs);
      break;
    case 13:
      printf("Binary value of configuration jumpers = 1101.\n\n", crobs);
      break;
    case 14:
      printf("Binary value of configuration jumpers = 1110.\n\n", crobs);
      break;
    case 15:
      printf("Binary value of configuration jumpers = 1111.\n\n", crobs);
      break;
    default:
      strcpy(errmsg, 
             "Value of configuration jumpers was not translated correctly.");
      return('q');
      break;
    }
    delay(10000);
    if (pause() == 'q') {
      strcpy(errmsg, tstname);
      strcat(errmsg, " terminated by user.");
      return('q');
    }
    if (i >= INFINITY) { /* Since the test is to     */
      i=(-1);            /* run forever, reset 'i'.  */
    }
  }
  return(0);
}


