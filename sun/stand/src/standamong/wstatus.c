/*
 *  static char     fpasccsid[] = "@(#)wstatus.c 1.1 2/14/86 Copyright Sun Microsystems";
 */ 
#include <sys/types.h>
#include "fpa.h"



/*
 * This test causes the Weitek chips to produce every possible
 * value at the S+ outputs.  These bits are then observed via
 * the WSTATUS register.  The ALU and multiplier are distinguished 
 * by the instruction address
 */
#define    zero_sp    0x00000000
#define    zero_msw   0x00000000
#define    zero_lsw   0x00000000

#define    half_sp    0x3f000000
#define    half_msw   0x3fe00000
#define    half_lsw   0x00000000

#define    one_sp    0x3f800000
#define    one_msw    0x3ff00000
#define    one_lsw    0x00000000

#define    two_sp    0x40000000
#define    two_msw   0x40000000
#define    two_lsw   0x00000000

#define    pi_sp    0x40490fdb
#define    pi_msw   0x400921fb
#define    pi_lsw   0x54442d18

#define    pi_4_sp  0x3f490fdb
#define    pi_4_msw 0x3fe921fb
#define    pi_4_lsw 0x54442d18

#define    inf_sp   0x7f800000
#define    inf_msw  0x7ff00000
#define    inf_lsw  0x00000000

#define    nan_sp   0x7fbfffff
#define    nan_msw  0x7ff7ffff
#define    nan_lsw  0xffffffff

#define    denorm_sp    0x00000001
#define    denorm_msw   0x00000000
#define    denorm_lsw   0x00000001

#define    maxn_sp   0x7f7fffff
#define    maxn_msw  0x7fefffff
#define    maxn_lsw  0xffffffff

#define    minn_sp   0x00800000
#define    minn_msw  0x00100000
#define    minn_lsw  0x00000000

#define    min1_sp   0x00800001
#define    min1_msw  0x00100001
#define    min1_lsw  0x00010001

#define    maxd_sp   0x007fffff
#define    maxd_msw  0x000fffff
#define    maxd_lsw  0xffffffff

#define    nocare  0
#define    base	     0xe0000000

#define    add_sp   (base + 0xa80)
#define    add_dp   (base + 0xa84)
#define    div_sp   (base + 0xa30)
#define    div_dp   (base + 0xa34)
#define    mult_sp  (base + 0xa08)
#define    mult_dp  (base + 0xa0c)


struct testws {

     u_long   a_msw;
     u_long   a_lsw;
     u_long   b_msw;
     u_long   b_lsw;
     u_long   instr;
     u_long   status;
};

struct testws test_ws[] = {
   /* a_msw       a_lsw       b_msw       b_lsw      instr     status*/
                                                                        /*ALU*/
    {  zero_sp,    nocare,     zero_sp,    nocare,     add_sp,     0x0 },  /*zero,ex*/
    {  zero_msw,   zero_lsw,   zero_msw,   zero_lsw,   add_dp,     0x0 },
    {  inf_sp,     nocare,     inf_sp,     nocare,     add_sp,     0x1 },  /*inf,ex*/
    {  inf_msw,    inf_lsw,    inf_msw,    inf_lsw,    add_dp,     0x1 },
    {  one_sp,     nocare,     one_sp,     nocare,     add_sp,     0x2 },  /*fin,ex*/
    {  one_msw,    one_lsw,    one_msw,    one_lsw,    add_dp,     0x2 },
    {  one_sp,      nocare,     pi_sp,     nocare,     div_sp,     0x3 },  /*fin,inex*/
    {  one_msw,    one_lsw,    pi_msw,     pi_lsw,     div_dp,     0x3 },
      /* status 0x4 is unused on both chips */                         /*unused*/
    {  maxn_sp,    nocare,     maxn_sp,    nocare,     add_sp,     0x5 },  /*ovfl,inex*/
    {  maxn_msw,   maxn_lsw,   maxn_msw,   maxn_lsw,   add_dp,     0x5 },
    {  minn_sp,    nocare,     two_sp,     nocare,     div_sp,     0x6 },  /*unfl*/
    {  minn_msw,   minn_lsw,   two_msw,    two_lsw,    div_dp,     0x6 },  
    {  minn_sp,    nocare,     pi_sp,      nocare,     div_sp,     0x7 },  /*unfl,inex*/
    {  minn_msw,   minn_lsw,   pi_msw,     pi_lsw,     div_dp,     0x7 },
    {  maxd_sp,    nocare,     two_sp,     nocare,     div_sp,     0x8 },  /*a-denorm*/
    {  maxd_msw,   maxd_lsw,   two_msw,    two_lsw,    div_dp,     0x8 },
    {  two_sp,     nocare,     maxd_sp,    nocare,     div_sp,     0x9 },  /*b-denorm*/
    {  one_msw,    one_lsw,    maxd_msw,   maxd_lsw,   div_dp,     0x9 },
    {  maxd_sp,    nocare,     maxd_sp,    nocare,     div_sp,     0xa },  /*ab-denorm*/
    {  maxd_msw,   maxd_lsw,   maxd_msw,   maxd_lsw,   div_dp,     0xa },
    {  one_sp,     nocare,     zero_sp,    nocare,     div_sp,     0xb },  /* div 0*/
    {  one_msw,    one_lsw,    zero_msw,   zero_lsw,   div_dp,     0xb },
    {  nan_sp,     nocare,     zero_sp,    nocare,     add_sp,     0xc },  /*a-nan*/
    {  nan_msw,    nan_lsw,    zero_msw,   zero_lsw,   add_dp,     0xc }, 
    {  zero_sp,    nocare,     nan_sp,     nocare,     add_sp,     0xd },  /*b-nan*/
    {  zero_sp,    nocare,     nan_msw,    nan_lsw,    add_dp,     0xd },
    {  nan_sp,     nocare,     nan_sp,     nocare,     add_sp,     0xe },  /*ab-nan*/
    {  nan_msw,    nan_lsw,    nan_msw,    nan_lsw,    add_dp,     0xe },
    {  inf_sp,     nocare,     inf_sp,     nocare,     div_sp,     0xf },  /*invalid*/
    {  inf_msw,    inf_lsw,    inf_msw,    inf_lsw,    div_dp,     0xf },
                               /*MULT*/
    {  zero_sp,    nocare,     zero_sp,    nocare,     mult_sp,    0x0 },  /*zero,ex*/   
    {  zero_msw,   zero_lsw,   zero_msw,   zero_lsw,   mult_dp,    0x0 },
    {  inf_sp,     nocare,     inf_sp,     nocare,     mult_sp,    0x1 },  /*inf,ex*/
    {  inf_msw,    inf_lsw,    inf_msw,    inf_lsw,    mult_dp,    0x1 },
    {  one_sp,     nocare,     one_sp,     nocare,     mult_sp,    0x2 },  /*fin,ex*/
    {  one_msw,    one_lsw,    one_msw,    one_lsw,    mult_dp,    0x2 },
    {  pi_sp,      nocare,     pi_sp,      nocare,     mult_sp,    0x3 },  /*fin,inex*/
    {  pi_msw,     pi_lsw,     pi_msw,     pi_lsw,     mult_dp,    0x3 },
      /* status 0x4 is unused on both chips */
    {  maxn_sp,    nocare,     maxn_sp,    nocare,     mult_sp,    0x5 },  /*ovfl,inex*/
    {  maxn_msw,   maxn_lsw,   maxn_msw,   maxn_lsw,   mult_dp,    0x5 },
    {  minn_sp,    nocare,     half_sp,    nocare,     mult_sp,    0x6 },  /*unfl*/
    {  minn_msw,   minn_lsw,   half_msw,   half_lsw,   mult_dp,    0x6 },  
    {  min1_sp,    nocare,     pi_4_sp,    nocare,     mult_sp,    0x7 },  /*unfl,inex*/
    {  min1_msw,   min1_lsw,   pi_4_msw,   pi_4_lsw,   mult_dp,    0x7 },
    {  maxd_sp,    nocare,     half_sp,    nocare,     mult_sp,    0x8 },  /*a-denorm*/
    {  maxd_msw,   maxd_lsw,   half_msw,   half_lsw,   mult_dp,    0x8 },
    {  half_sp,    nocare,     maxd_sp,    nocare,     mult_sp,    0x9 },  /*b-denorm*/
    {  half_msw,   half_lsw,   maxd_msw,   maxd_lsw,   mult_dp,    0x9 },
    {  denorm_sp,  nocare,     denorm_sp,  nocare,     mult_sp,    0xa },  /*ab-denorm*/
    {  denorm_msw, denorm_lsw, denorm_msw, denorm_lsw, mult_dp,    0xa },
      /* status 0xb is  divide by zero and is unused by the MULT */
    {  nan_sp,     nocare,     zero_sp,    nocare,     mult_sp,    0xc },  /*a-nan*/
    {  nan_msw,    nan_lsw,    zero_msw,   zero_lsw,   mult_dp,    0xc }, 
    {  zero_sp,    nocare,     nan_sp,     nocare,     mult_sp,    0xd },  /*b-nan*/
    {  zero_sp,    nocare,     nan_msw,    nan_lsw,    mult_dp,    0xd },
    {  nan_sp,     nocare,     nan_sp,     nocare,     mult_sp,    0xe },  /*ab-nan*/
    {  nan_msw,    nan_lsw,    nan_msw,    nan_lsw,    mult_dp,    0xe },
    {  inf_sp,     nocare,     zero_sp,    nocare,     mult_sp,    0xf },  /*invalid*/
    {  inf_msw,    inf_lsw,    zero_msw,   zero_lsw,   mult_dp,    0xf },
    {       00,         00,         000,        000,      0000,    0x0 }
};

      
fpa_ws()
{
	u_long  i, cmp_status, tmp_var;
	u_long  *ptr, *soft_clear, *wstatus;
	u_long  amsw, alsw, bmsw, blsw;

   /* Test both single and double precision */



	soft_clear = (u_long *)FPA_CLEAR_PIPE_PTR;
	/* stable only, do not read others get bus error */
	wstatus = (u_long *)(FPA_BASE + FPA_WSTATUSS);

	/* clear the hard pipe */
	*(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;

	/* initialize by giving the diagnostic initialize command */
	*(u_long *)DIAG_INIT_CMD = 0x0;
	*(u_long *)MODE_WRITE_REGISTER = 0x2;

	for (i=0; test_ws[i].instr!=0; i++) {

		amsw = test_ws[i].a_msw; /* for the sake of assembler */
		alsw = test_ws[i].a_lsw;
		bmsw = test_ws[i].b_msw; 
		blsw = test_ws[i].b_lsw;

		*(u_long *)REGISTER_ZERO_MSW = amsw;    /* reg0 = operand A */
		*(u_long *)REGISTER_ZERO_LSW = alsw;
		*(u_long *)REGISTER_ONE_MSW = bmsw;    /* reg1 = operand B */
		*(u_long *)REGISTER_ONE_LSW = blsw;

		tmp_var = test_ws[i].instr;      /* for the sake of assembler */
		ptr = (u_long *)tmp_var;
		*ptr = 0x10002;                  /* reg2 = reg0 op reg1 */
		cmp_status = (*wstatus >> 8) & 0xf;
		if (cmp_status != test_ws[i].status) 
			return(-1); 
		
		*soft_clear = 0;
		        /* initialize by giving the diagnostic initialize command */ 
                *(u_long *)DIAG_INIT_CMD = 0x0; 
	        *(u_long *)MODE_WRITE_REGISTER = 0x2; 

	} 
	return(0);
}

