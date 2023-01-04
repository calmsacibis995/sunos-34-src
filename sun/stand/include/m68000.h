/*	@(#) m68000.h 1.1 86/09/25 Copyright Sun Micro */

#define M68000

/*                      68000-specific C Enhancements
                                V.R. Pratt
                                Jan., 1981

This header file permits C programs to operate on parts of the 68000 for 
which the C language has no names.  It is quite specific both to the 68000 
and to the MIT 68000 assembler.

Anyone planning to use asm's in their C code should consider using the macros
in this library in order to put one layer of abstraction between their code and
the compiler.  When the compiler and the assembler change it will be easier to
change this file than to change all code containing asm's.

The externs at the beginning permit all direct data moves to be expressed as C
assignments, e.g. temp = cc, sr = temp, etc., regardless of whether temp is a
register automatic, external, or what.  Thus if you had intended to write 
asm("   movl    x,a6"), using m68000.h you can write it as a6 = x; .  Remember
that usp may only be stored to and from a-registers, and that cc may only be
stored to, not from (use sr for from).  The data registers are declared of
type int, the sr and cc are short, and the rest are int*.  This should
minimize the need for casts.  Note that the cast used in d3 = (int)a5 has the
unfortunate side-effect of moving a5 to d0 then d0 to d3; this may be
prevented if necessary with *(int**)&d3 = a5, though setm(d3,a5) may be more
readable.  

The remainder of this file consists of macros each generating one or
two asm constructs.  For example the pushm(x) macro pushes onto the stack the
registers identified by x, using a single moveml instruction.  Thus
pushm(D2+A3) will push those two registers onto the stack with a single moveml.
pushm(A5-D2) also works; it means push everything from A5 exclusive to D2
inclusive (sorry about that, but including A5 involves an order of magnitude
increase in labor).

*/

#define TRUE 1
#define FALSE 0

extern d0,d1,d2,d3,d4,d5,d6,d7,
       *a0,*a1,*a2,*a3,*a4,*a5,*a6,*a7,*sp,*usp;
extern short sr,cc;

/* Basic asm forms */
#define zerop(opr)      {asm("  opr");}
#define unop(opr,opd)   {asm("  opr     opd");}
#define binop(opr,o1,o2){asm("  opr     o1,o2");}

/* This does not work with the Unisoft compiler  
 * #define label(lab)      {{extern lab();} asm("  .globl  lab"); asm("lab:");}
 */

/* Data motion */
#define move(s,x,y)     binop(mov/**/s,x,y)

/* set - closer to ordinary assignment than move */
#define set(x,y)        move(l,y,x)
#define setw(x,y)       move(w,y,x)
#define setb(x,y)       move(b,y,x)
#define setm(x,y)       move(eml,y,x)           /* for moveml */
#define setp(x,y)       move(epl,y,x)           /* for movepl */

#define clear(x)        unop(clrl,x)
#define clearw(x)       unop(clrw,x)
#define clearb(x)       unop(clrb,x)

#define push(x)         set(sp@-,x)
#define pushw(x)        setw(sp@-,x)
#define pushb(x)        setb(sp@-,x)
#define pushm(x)        setm(sp@-,#![x])/* ! reverses the bits automatically */

#define pop(x)          set(x,sp@+)
#define popw(x)         setw(x,sp@+)
#define popb(x)         setb(x,sp@+)
#define popm(x)         setm(#x,sp@+)

/* Data operations */
#define swap(x)         {asm("  swap    x");}
#define neg(x)          unop(negl,x)
#define negs(x)         unop(negw,x)
#define negc(x)         unop(negb,x)
#define add(x,y)        binop(addl,y,x)  /*  x += y  */
#define adds(x,y)       binop(addw,y,x)  /*  x += y  */
#define addc(x,y)       binop(addb,y,x)  /*  x += y  */
#define sub(x,y)        binop(subl,y,x)  /*  x += y  */
#define subs(x,y)       binop(subw,y,x)  /*  x += y  */
#define subb(x,y)       binop(subb,y,x)  /*  x += y  */
#define mult(x,y)       binop(muls,y,x)  /*  x *= y  */
#define umult(x,y)      binop(mulu,y,x)  /*  x *= y  (unsigned) */
#define div(x,y)        binop(divs,y,x)  /*  x /= y  */
#define udiv(x,y)       binop(divu,y,x)  /*  x /= y  (unsigned) */
#define rem(x,y)        {binop(divs,y,x); /*  x %= y  */\
                         swap(x)}
#define urem(x,y)       {binop(divu,y,x); /*  x %= y  (unsigned) */\
                         swap(x)}
#define rotate(x,y)     binop(roll,y,x)  /*  rotate x by y */
#define rrotate(x,y)    binop(rorl,y,x)  /*  reverse rotate x by y */

/* Status register operations */
#define intlevel00(i)   {binop(orw,#0x0700,sr);  /* aux for intlevel */\
                         binop(andw,#0x/**/i,sr);}
#define intlevel(i)     intlevel00(F/**/i/**/FF)

/* Control constructs */
#define go(x)           unop(jra,x)
#define call(x)         unop(jbsr,x)
#define trap(n)         {asm("  trap    #n");}
#define rts             {asm("  rts");}
#define returns(x)      {d0 = (int)(x); rts}
#define rtr             {asm("  rtr");}
#define returnr(x)      {d0 = (int)(x); rtr}
#define rte             {asm("  rte");}
#define returne(x)      {d0 = (int)(x); rte}
#define subroutine(x,y) label(x)  y rts;
#define sysroutine(x,y) label(x)  y rte;
