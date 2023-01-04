

/* ======================================================================
   Author: Peter Costello
   Date :  October 21, 1982
   Purpose:  Include file for cdiag.c
   ====================================================================== */

#include "colorbuf.h"
#include "reentrant.h"
#include "buserror.h"
#include "m68000.h"
#include "vectors.h"

#define NULL 0		/* NULL Pointers */
#define NIL  0

/* Define structure used to link together base addresses of multiple color
   boards. */

#define Num_Tests 9	/* Number of categories to test on board. */
struct bd_list {
       int base;	/* Base address of board (Lies on a 16k boundary). */
       short device;    /* Color board number in system. */
       int error[Num_Tests];
       struct bd_list *next;
		}; 

extern struct bd_list *head;

int CGXBase;		/* Base address of color board. */

#define Sec1 150000	/* Decrementing this takes 1 second. */

#define Addr_Space 0x200000	/* Upto 2 MB of physical memory allowed */
#define Max_Boards 12		/* Maximum of 12 color boards in one sys */


