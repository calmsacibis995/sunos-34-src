/* ====================REENTRANT FUNCTION CALLS ON THE 68000==============

Modified 20Apr82 by JCGilmore to correspond to Unisoft C conventions
(leading _ on all external C names).


Author:         V.R.Pratt
Date:           Sept. 7, 1980.

This package permits the use of the expression
        reentrant(fun)
in place of the usual way to begin a function definition, namely
        fun()

Invoking reentrant(fun) generates a selfcontained function named fun which
pushes d0,d1,a0,a1 on the stack, calls _fun, then pops a1,a0,d1,d0 back off the
stack and does a rte (return from exception).  

Example usage:

reentrant(KbdServ)
{return(ACIA1Data&0177);}

*/

#define REENTRANT

#define reentrant(fun) reentrant_(_/**/fun,__/**/fun)

#define reentrant_(fun,_fun)\
asm("   .text ");\
asm("   .globl  fun");\
asm("fun:");\
asm("   moveml  #0xc0c0,sp@-");\
asm("   jsr     _fun");\
asm("   moveml  sp@+,#0x0303");\
asm("   rte");\
int fun();\
fun()
