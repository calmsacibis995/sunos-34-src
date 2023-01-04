

/* ======================================================================
   Author: Peter Costello
   Date :  April 15, 1983
   Purpose:  Include file for cdiag.c
   ====================================================================== */
/*	@(#)sc.diag.h 1.1 9/25/86 Copyright Sun Microsystems, Inc. */

#include "scbuf.h"
#include "../include/reentrant.h"
#include "../include/m68000.h"
#include "../include/vectors.h"

#define NULL 0		/* NULL Pointers */
#define NIL  0

#define Num_Tests 29 /* Number of categories to test on board. */

/* Define structure used to link together base addresses of multiple color
   boards. */
struct bd_list {
       int base;	/* Base address of board (Lies on a 16k boundary). */
       short device;    /* Color board number in system. */
       int error[Num_Tests];
       short res_1k1k;
       struct bd_list *next;
		}; 

extern struct bd_list *head;
extern char *error_str[];
extern int SCBase;
extern int SCWidth;
extern int SCHeight;

#define Sec1 150000	/* Decrementing this takes 1 second. */

#define Addr_Space 0x1000000	/* Upto 16 MB of physical memory allowed */

#define Max_Boards 4		/* Maximum of 4 color boards in one sys */
#define Planes	   8		/* Currently eight planes */

#define print_xy(addr)						\
      { int x1;							\
        short xx,yy;						\
	x1 = ((int)addr) - SCBase - 0x100000;			\
	yy = x1 / SCWidth;					\
	xx = x1 % SCWidth;					\
	printf("Addr 0x%x => 0x%x. X = %d. Y = %d.\n",		\
		((long)addr),x1,xx,yy);				\
      }

