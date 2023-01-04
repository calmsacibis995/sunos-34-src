/******************************************************************************
*
*	name		"expars.c"
*
*	synopsis	status = expars(source, destination, limit)
*
*			status	=>	0 = success, non-zero = error
*
*			source	=>	set of arguments or ranges to be
*					parsed and expanded
*
*			destination =>	address of buffer to hold expanded
*					list of arguments
*
*			limit	=>	size of expand buffer
*
*
*
*	description	A single string (usually a command line argument) is
*			is parsed and expanded according to its content. If
*			a range is given, a table of device names is
*			 constructed.  The same applies to a a list.
*			  Construction  uses the full root, replacing
*			each instance with the number of bytes given in the
*			range or list.  Example:
*
*				"/dev/tty00-3"
*
*			expands to:
*
*				/dev/tty00
*				/dev/tty01
*				/dev/tty02
*				/dev/tty03
*
*			and
*				"/dev/tty00,2,7,11"
*
*			expands to:
*
*				/dev/tty00
*				/dev/tty02
*				/dev/tty07
*				/dev/tty11
*
*
*			conditions can be combined:
*
*				"/dev/tty00-4,8,a,c-e"
*
*			expands to:
*			
*
*				/dev/tty00
*				/dev/tty01
*				/dev/tty03
*				/dev/tty04
*				/dev/tty08
*				/dev/tty0a
*				/dev/tty0c
*				/dev/tty0d
*				/dev/tty0e
*
*/

#ifndef lint
static  char sccsid[] = "@(#)expars.c 1.1 86/09/25 SMI"; 
#endif
 
#include <stdio.h>
#include <ctype.h>

/*
*	Globals
*/

int	bleft;				/* # of bytes left in expand buffer */
char	*srcbuf;			/* address of source line */
char	*exbuf;				/* address of expand buffer */
char 	*root;				/* address of most recent root name */

int	overflow();			/* overflow checking routine */
int	exrange();			/* range expansion routine */
int	exlist();			/* list expansion routine */
int	strinc();			/* increment a string routine */

/*
*	Main program
*/

expars(compact,expand,limit)
  char	*compact,*expand;		/* ptrs source and expand buffers */
  int	limit;				/* size of expand buffer */

{
  int i;				/* general purpose loop variable */
  int status;				/* error status */
  unsigned char c,*ptr;			/* general character and pointer */


/*
*	Initialize
*/

  status = 0;				/* assume no error */
  bleft = limit-2;			/* set bytes left less a margin */
  srcbuf = compact;			/* global ptr to source line */
  exbuf = expand;			/* global ptr to expand buffer */
  root = expand;			/* intial root name at expand buffer */
  *expand = '\0';			/* set "null" expand buffer */

/*
*	Parse source line
*/

    while ((*exbuf++ = *srcbuf++) != '\0') {
					/* source line is '\0' terminated */
      if (--bleft == 0) status = -1;	/* prevent buffer overflow */
      
      switch (*(srcbuf-1)) {

        case '-':			/* range list */
          status = exrange();
          break;

        case ',':			/* item list */
          status = exlist();
          break;
      }
      if (status) break;		/* error, stop parsing */
    }
  if (!status) *exbuf = '\0';		/* mark end of buffer */             
  return(status);			/* exit with status */
}

/*
*	Expand range
*/

exrange()
{
  int	upsize;				/* size of upper limit */

  *(exbuf-1) = '\0';			/* tag end root name (overwrite '-') */
  for(upsize=0; (isalnum(*srcbuf++)); upsize++);
					/* get size of upper limit */
  if (upsize == 0) return(-1);		/* no upper limit, return error */
  while ((strncmp(srcbuf-1-upsize,exbuf-1-upsize,upsize)) > 0) {
					/* expand until ranges meet */
    while ((*exbuf++ = *root++) != '\0') {
					/* copy previous root name */
    if (--bleft == 0) return(-1);	/* prevent buffer overflow */
    }
    strinc(exbuf-1-upsize);		/* yet upper limit, increment */
  }
  srcbuf--;				/* end of upper range in source line */
  exbuf--;				/* end of latest name in buffer */
  return(0);				/* return success */
}

/*
*	Expand list
*/

exlist()
{
  int	newsize;			/* size of new item */

  *(exbuf-1) = '\0';			/* tag end root name (overwrite ',') */
  for(newsize=0; (isalnum(*srcbuf++)); newsize++);
					/* get size of new item */
  if (newsize == 0) return(-1);		/* no new item, return error */
  while ((*exbuf++ = *root++) != '\0') {
					/* copy previous root name */
    if (--bleft == 0) return(-1);	/* prevent buffer overflow */
  }
  srcbuf -= newsize + 1;		/* point at new item replacement */
  exbuf -= newsize + 1;			/* point at old part to replace */
  while (newsize--) {
    *exbuf++ = *srcbuf++;		/* replace old part of item name */
  }
  return(0);				/* return success */
}


/*
*	Increment a string
*/

strinc(ptr)
char	*ptr;				/* string to increment */
{
  int	size;				/* size of string */

  for (size=0; *ptr++ != '\0'; size++);	/* measure string */
  ptr--;				/* back up to end of string */
  while(size--) {
    ptr--;				/* back up to next character */
    (*ptr)++;				/* increment last character */

    switch (*ptr) {			/* check for "rollover" of range */
      case '9'+1:			/* end of numeric range? */
        *ptr = '0';			/* "roll over" for carry */
        break;				/* try again on next char */
      case 'Z'+1:			/* end of upper case alpha range? */
        *ptr = 'A';			/* "roll over" for carry */
        break;				/* try again on next char */
      case 'z'+1:			/* end of lower case range? */
        *ptr = 'a';			/* "roll over" for carry */
        break;				/* try again on next char */
      default:
        return(0);			/* done, exit */
    }
  }
  return(-1);				/* no more places for carry, error */
}
