/*
 *      static char     fpasccsid[] = "@(#)lock.c 1.2 3/6/86 Copyright Sun Microsystems";
 *
 *      Copyright (c) 1985 by Sun Microsystems, Inc 
 *
 */
#include <sys/types.h>
#include "fpa.h"

struct registers
{
        u_long  reg_msw;
        u_long  reg_lsw;
};
struct shadow_regs
{
        u_long shreg_msw;
        u_long shreg_lsw;
};

struct registers user[] = {

        0xE0000C00,   0xE0000C04,
        0xE0000C08,   0xE0000C0C, 
        0xE0000C10,   0xE0000C14,
        0xE0000C18,   0xE0000C1C,
        0xE0000C20,   0xE0000C24,  
        0xE0000C28,   0xE0000C2C,   
        0xE0000C30,   0xE0000C34, 
        0xE0000C38,   0xE0000C3C,
        0xE0000C40,   0xE0000C44,
        0xE0000C48,   0xE0000C4C, 
        0xE0000C50,   0xE0000C54,
        0xE0000C58,   0xE0000C5C,
        0xE0000C60,   0xE0000C64,
        0xE0000C68,   0xE0000C6C,
        0xE0000C70,   0xE0000C74, 
        0xE0000C78,   0xE0000C7C,
        0xE0000C80,   0xE0000C84, 
        0xE0000C88,   0xE0000C8C, 
        0xE0000C90,   0xE0000C94, 
        0xE0000C98,   0xE0000C9C, 
        0xE0000CA0,   0xE0000CA4, 
        0xE0000CA8,   0xE0000CAC, 
        0xE0000CB0,   0xE0000CB4, 
        0xE0000CB8,   0xE0000CBC, 
        0xE0000CC0,   0xE0000CC4, 
        0xE0000CC8,   0xE0000CCC, 
        0xE0000CD0,   0xE0000CD4, 
        0xE0000CD8,   0xE0000CDC, 
        0xE0000CE0,   0xE0000CE4, 
        0xE0000CE8,   0xE0000CEC, 
        0xE0000CF0,   0xE0000CF4, 
        0xE0000CF8,   0xE0000CFC 
};

struct shadow_regs shadow[] = {
         
        0xE0000E00,   0xE0000E04,
        0xE0000E08,   0xE0000E0C,
        0xE0000E10,   0xE0000E14,
        0xE0000E18,   0xE0000E1C,
        0xE0000E20,   0xE0000E24,
        0xE0000E28,   0xE0000E2C,
        0xE0000E30,   0xE0000E34,
        0xE0000E38,   0xE0000E3C
};


struct dp_short
{
        u_long   addr;
};
/*
struct dp_short dps[] = {

        0xE0000384,
        0xE000038C, 
        0xE0000394, 
        0xE000039C, 
        0xE00003A4, 
        0xE00003AC,   
        0xE00003B4,  
        0xE00003BC, 
        0xE00003C4, 
        0xE00003CC, 
        0xE00003D4,
        0xE00003DC, 
        0xE00003E4, 
        0xE00003EC, 
        0xE00003F4, 
        0xE00003FC 
};
*/
struct dp_short dps[] = {

	0xE0000504,
        0xE000050C,
        0xE0000514,
        0xE000051C,
        0xE0000524,
        0xE000052C,   
        0xE0000534,  
        0xE000053C, 
        0xE0000544,
        0xE000054C,
        0xE0000554,
        0xE000055C,
        0xE0000564,
        0xE000056C,
        0xE0000574,
        0xE000057C 
};


struct dp_ext 
{
	u_long	addr;
	u_long  addr_lsw;
};

struct dp_ext  ext[] = {

	0xE0001484,   0xE0001800,
        0xE000148C,   0xE0001808, 
        0xE0001494,   0xE0001810, 
        0xE000149C,   0xE0001818, 
        0xE00014A4,   0xE0001820, 
        0xE00014AC,   0xE0001828, 
        0xE00014B4,   0xE0001830, 
        0xE00014BC,   0xE0001838, 
        0xE00014C4,   0xE0001840, 
        0xE00014CC,   0xE0001848, 
        0xE00014D4,   0xE0001850, 
        0xE00014DC,   0xE0001858, 
        0xE00014E4,   0xE0001860, 
        0xE00014EC,   0xE0001868, 
        0xE00014F4,   0xE0001870, 
        0xE00014FC,   0xE0001878 
};

struct dp_cmd
{
	u_long	data;
};
struct dp_cmd nxt_cmd[] = {

	0x000,
	0x041,
	0x082,
	0x0C3,
	0x104,
	0x145,
	0x186,
	0x1C7,
	0x208,
	0x249,
	0x28A,
	0x2CB,
	0x30C,
	0x34D,
	0x38E,
	0x3CF,
	0x410,
	0x452,
	0x493,
	0x4D4,
	0x515,
	0x556,
	0x597,
	0x5D8,
	0x619,
	0x65A,
	0x69B,
	0x6DC,
	0x71D,
	0x75E,
	0x79F
};
struct dp_cmd cmd[] = {
	
	0x020040,
	0x030081,
	0x0400C2,
	0x050103,
	0x060144,
	0x070185,
	0x0801C6,
	0x090207,
	0x0A0248,
	0x0B0289,
	0x0C02CA,
	0x0D030B,
	0x0E034C,
	0x0F038D,
	0x1003CE,
	0x11040F,
	0x120450,
	0x130491,
	0x1404D2,
	0x150513,
	0x160554,
	0x170595,
	0x1805D6,
	0x190617,
	0x1A0658,
	0x1B0699,
	0x1C06DA,
	0x1D071B,
	0x1E075C,
	0x1F079D
};
lock_test()
{

	/* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;

	*(u_long *)FPA_IMASK_PTR = 0x1;
	if (dp_short_test())
		return(-1);
	if (dp_ext_test())
		return(-1);
	if (dp_cmd_test())
		return(-1);	
	if (next_dp_short_test())
		return(-1);
	if (next_dp_ext_test())
		return(-1);
	if (next_dp_cmd_test())
		return(-1);

	return(0);
}

next_dp_short_test()
{
        u_long  res_a_msw, res_a_lsw, res_n_msw, res_n_lsw, res_i_msw, res_i_lsw;
        int     i, j, k;
	u_long  res0_msw, res0_lsw, shad_res_msw, shad_res_lsw, value_i, value_0;
        u_long  *soft_clear, *reg0, *reg0_lsw, *reg0_addr, *reg0_addr_lsw;
	u_long  *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw, *shadow_j, *shadow_j_lsw;
	u_long  *ptr4_lsw, *ptr4, *ptr5_lsw, *ptr5;
	u_long  res4_msw, res5_msw, res4_lsw, res5_lsw;	
        u_long  *ptr2, *ptr2_lsw;
 
        soft_clear = (u_long *)FPA_CLEAR_PIPE_PTR;
        for (i = 0; i < 16; i++)
        {
                ptr2 = (u_long *)user[i].reg_msw;  
                ptr2_lsw = (u_long *)user[i].reg_lsw; 
                 
                /* initialize */
                *ptr2 = 0x0;
                *ptr2_lsw = 0x0;
        }
        res_a_msw = 0x3FD55555; /* the result is always active - dp value 0.33333333333 */
        res_a_lsw = 0x55555555;
	res_n_msw = 0x3FE55555; /* the result of next should be always be 0.6666666666 */
	res_n_lsw = 0x55555555;
	value_0 = 0x3FF00000;
	value_i = 0x40000000;
	reg0 = (u_long *)REGISTER_ZERO_MSW; /* always this register will be active */
	reg0_lsw = (u_long *)REGISTER_ZERO_LSW;

        reg0_addr = (u_long *)dps[0].addr; /* for higher significant value */
        reg0_addr_lsw = (u_long *)0xE0001000;  /* for least significant value */

        for (i = 0; i < 16; i++)
        {

                 reg_i_addr = (u_long *)dps[i].addr; /* for higher significant value */ 
                 reg_i_addr_lsw = (u_long *)0xE0001000;  /* for least significant value */     
                 reg_i = (u_long *)user[i].reg_msw; 
                 reg_i_lsw = (u_long *)user[i].reg_lsw;
                 for (j = 0; j < 8; j++)
                 {

                        shadow_j = (u_long *)shadow[j].shreg_msw;
                        shadow_j_lsw = (u_long *)shadow[j].shreg_lsw;
              		*reg0 = 0x3FF00000;  /* register 0 has dp value of 1 - active stage */
			*reg0_lsw = 0x0; 
			*reg_i = 0x40000000; /* register has dp value of 2 for next stage2 for next stage */
			*reg_i_lsw = 0x0;

			*(u_long *)FPA_IMASK_PTR = 0x1;

                        *reg0_addr = 0x40080000; /* operand has dp value of 3 active stage */
                        *reg0_addr_lsw = 0x0;
			*reg_i_addr = 0x40080000; /* operand  has dp value 3 next stage */
                        *reg_i_addr_lsw = 0x0;
		
			/* may be soft clear */	
                        shad_res_msw = *shadow_j; /* read the shadow register */
                        shad_res_lsw = *shadow_j_lsw;
                        *soft_clear = 0x0;

                        res_i_msw = *reg_i; /* read the result from the reg  i*/
                        res_i_lsw = *reg_i_lsw;
			res0_msw = *reg0;
			res0_lsw = *reg0_lsw;
			
			*(u_long *)FPA_IMASK_PTR = 0x0;

			if ((j == 0) && (j == i))
			{
			if ((shad_res_msw !=0x3FCC71C7)||(res_i_msw!=0x3FCC71C7)||(res0_msw != 0x3FCC71C7))

/*	if ((shad_res_msw != res_n_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_n_msw)) */
			{
			
                        /*                printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
                                                i, j, res0_msw, shad_res_msw);
					printf("res0_lsw = %x, shad_lsw = %x, resi_msw = %x, resi_lsw = %x\n",
						res0_lsw, shad_res_lsw, res_i_msw, res_i_lsw);
                         */
		               return(-1); 
                        }
			}

			else                       if ((j == 0) && (j != i))
			{
		if ((shad_res_msw != res_a_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw))
			{
			
                         /*               printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n", 
                                                i, j, res0_msw, shad_res_msw);
					printf("res_i_msw = %x\n", res_i_msw);
                          */
		              return(-1); 
                        }
                        }
			else if((j != 0) && (j == i)) {
               			 
		if ((shad_res_msw != res_n_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw))
			{
		/*	printf("Err3:reg = %x, shadow = %x, rres= %x, sres = %x\n",
                                    i, j, res0_msw, shad_res_msw);
			printf("res_i_msw = %x\n", res_i_msw);
                 */
	                       return(-1);
                        }
                        }
			else if ((j != 0) && (j != i)) {
				if (i == 0) {
		if ((shad_res_msw != 0x0)|| (res_i_msw != value_i) || (res0_msw != value_i)) {
		/*	printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",
				i, j, res0_msw, shad_res_msw);
       		 	printf("res_i_msw = %x\n", res_i_msw);
                 */
	                       return(-1);  
                        }
			}
		else
			
		if ((shad_res_msw != 0x0)|| (res_i_msw != value_i) || (res0_msw != value_0))
			{
                  /*      printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",    
                                    i, j, res0_msw, shad_res_msw); 
			printf("res_i_msw = %x\n", res_i_msw);
                   */
	                     return(-1);    
                        }      
                        }  	
			*reg0 = 0x0;
			*reg0_lsw = 0x0;
			*reg_i = 0x0;
			*reg_i_lsw = 0x0;
		}
                *ptr2 = 0x0;
                *ptr2_lsw = 0x0;
        }
        return(0);

}
next_dp_ext_test()
{
        u_long  res_a_msw, res_a_lsw, res_n_msw, res_n_lsw, res_i_msw, res_i_lsw;
        int     i, j, k;
        u_long  res0_msw, res0_lsw, shad_res_msw, shad_res_lsw, value_i, value_0;
        u_long  *soft_clear, *reg0, *reg0_lsw, *reg0_addr, *reg0_addr_lsw;
        u_long  *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw, *shadow_j, *shadow_j_lsw;
        u_long  *ptr4_lsw, *ptr4, *ptr5_lsw, *ptr5;
        u_long  res4_msw, res5_msw, res4_lsw, res5_lsw;  
        u_long  *ptr2, *ptr2_lsw;
 
        soft_clear = (u_long *)FPA_CLEAR_PIPE_PTR;
        for (i = 0; i < 16; i++)
        {
                ptr2 = (u_long *)user[i].reg_msw;  
                ptr2_lsw = (u_long *)user[i].reg_lsw; 
                 
                /* initialize */
                *ptr2 = 0x0;
                *ptr2_lsw = 0x0;
        }
        res_a_msw = 0x3FD55555; /* the result is always active - dp value 0.33333333333 */
        res_a_lsw = 0x55555555;
        res_n_msw = 0x3FE55555; /* the result of next should be always be 0.6666666666 */
        res_n_lsw = 0x55555555;
        value_0 = 0x3FF00000;
        value_i = 0x40000000;
        reg0 = (u_long *)REGISTER_ZERO_MSW; /* always this register will be active */
        reg0_lsw = (u_long *)REGISTER_ZERO_LSW;

        reg0_addr = (u_long *)ext[0].addr; /* for higher significant value */ 
	reg0_addr_lsw = (u_long *)ext[0].addr_lsw; /* for least significant value */
        for (i = 0; i < 16; i++)
        {

                 reg_i_addr = (u_long *)ext[i].addr; /* for higher significant value */ 
                 reg_i_addr_lsw = (u_long *)ext[i].addr_lsw;  /* for least significant value */     
                 reg_i = (u_long *)user[i].reg_msw; 
                 reg_i_lsw = (u_long *)user[i].reg_lsw;
                 for (j = 0; j < 8; j++)
                 {

                        shadow_j = (u_long *)shadow[j].shreg_msw;
                        shadow_j_lsw = (u_long *)shadow[j].shreg_lsw;
                        *reg0 = 0x3FF00000;  /* register 0 has dp value of 1 - active stage */
                        *reg0_lsw = 0x0; 
                        *reg_i = 0x40000000; /* register has dp value of 2 for next stage2 for next stage */
                        *reg_i_lsw = 0x0;
                        *(u_long *)FPA_IMASK_PTR = 0x1;

                        *reg0_addr = 0x40080000; /* operand has dp value of 3 active stage */
                        *reg0_addr_lsw = 0x0;
                        *reg_i_addr = 0x40080000; /* operand  has dp value 3 next stage */
                        *reg_i_addr_lsw = 0x0;
                        /* may be soft clear */  
                        shad_res_msw = *shadow_j; /* read the shadow register */
                        shad_res_lsw = *shadow_j_lsw;
                        *soft_clear = 0x0;
                        res_i_msw = *reg_i; /* read the result from the reg  i*/
                        res_i_lsw = *reg_i_lsw;
                        res0_msw = *reg0;
                        res0_lsw = *reg0_lsw;
                        *(u_long *)FPA_IMASK_PTR = 0x0;

                        if ((j == 0) && (j == i))
                        {
                        if ((shad_res_msw !=0x3FCC71C7)||(res_i_msw!=0x3FCC71C7)||(res0_msw != 0x3FCC71C7))

/*      if ((shad_res_msw != res_n_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_n_msw)) */
                        {
                         
                                  /*      printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
                                                i, j, res0_msw, shad_res_msw);                                        printf("res0_lsw = %x, shad_lsw = %x, resi_msw = %x, resi_lsw = %x\n",
                                                res0_lsw, shad_res_lsw, res_i_msw, res_i_lsw);
                                   */
			     return(-1); 
                        }
                        }

                        else                       if ((j == 0) && (j != i))
                        {
                if ((shad_res_msw != res_a_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw))
                        {
                         
                             /*           printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n", 
                                                i, j, res0_msw, shad_res_msw);                   
		                        printf("res_i_msw = %x\n", res_i_msw);  
                              */
			          return(-1); 
                        }
                        }
                        else if((j != 0) && (j == i)) {
                                 
                if ((shad_res_msw != res_n_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw))
                        {
                       /*   printf("Err3:reg = %x, shadow = %x, rres= %x, sres = %x\n",
                                    i, j, res0_msw, shad_res_msw);
                        	printf("res_i_msw = %x\n", res_i_msw);
                        */
		                return(-1);
                        }
                        }
                        else if ((j != 0) && (j != i)) {
                                if (i == 0) {
                if ((shad_res_msw != 0x0)|| (res_i_msw != value_i) || (res0_msw != value_i)) {
                   /* 	printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",
                                i, j, res0_msw, shad_res_msw);
        		printf("res_i_msw = %x\n", res_i_msw);
                    */ 
		                return(-1);  
                        }
                        }
                else
                         
                if ((shad_res_msw != 0x0)|| (res_i_msw != value_i) || (res0_msw != value_0))
                        {
                   /*         printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",    
                                    i, j, res0_msw, shad_res_msw); 
        			printf("res_i_msw = %x\n", res_i_msw);
                     */
	                   return(-1);  
                        }      
                        }        
                        *reg0 = 0x0;
                        *reg0_lsw = 0x0;
                        *reg_i = 0x0;
                        *reg_i_lsw = 0x0;
                }
                *ptr2 = 0x0;
                *ptr2_lsw = 0x0;
        }
        return(0);

}
next_dp_cmd_test()
{
        u_long  res_a_msw, res_a_lsw, res_n_msw, res_n_lsw, res_i_msw, res_i_lsw;
        int     i, j, k;
        u_long  res0_msw, res0_lsw, shad_res_msw, shad_res_lsw, value_i, value_0;
        u_long  *soft_clear, *reg0, *reg0_lsw, *reg0_addr, *reg0_addr_lsw;
        u_long  *reg_i, *reg_i_lsw, *reg_i_addr, *reg_i_addr_lsw, *shadow_j, *shadow_j_lsw;
        u_long  *ptr4_lsw, *ptr4, *ptr5_lsw, *ptr5;
        u_long  res4_msw, res5_msw, res4_lsw, res5_lsw;  
        u_long  *ptr2, *ptr2_lsw;
 
        soft_clear = (u_long *)FPA_CLEAR_PIPE_PTR;
        for (i = 0; i < 32; i++)
        {
                ptr2 = (u_long *)user[i].reg_msw;  
                ptr2_lsw = (u_long *)user[i].reg_lsw; 
                 
                /* initialize */
                *ptr2 = 0x0;
                *ptr2_lsw = 0x0;
        }
        res_a_msw = 0x1; /* the result is always active - 0.333333333333 */
        res_a_lsw = 0x0;
        res_n_msw = 0x2; /* the result of next should be always be 0.6666666666 */     
        res_n_lsw = 0x0;
        value_0 = 0x1;
        value_i = 0x2;
        reg0 = (u_long *)REGISTER_ZERO_MSW; /* always this register will be active */
        reg0_lsw = (u_long *)REGISTER_ZERO_LSW;
	
	reg0_addr = (u_long *)0xE0000AC4;
	reg_i_addr =(u_long *)0xE0000AC4;
        for (i = 0; i < 31; i++)
        {

                 reg_i = (u_long *)user[i].reg_msw; 
                 reg_i_lsw = (u_long *)user[i].reg_lsw;
                 for (j = 0; j < 8; j++)
                 {
 
                        shadow_j = (u_long *)shadow[j].shreg_msw;
                        shadow_j_lsw = (u_long *)shadow[j].shreg_lsw;
                        *reg0 = value_0;  /* register 0 has dp value of 0.333 - active stage */
                        *reg0_lsw = 0x0; 
                        *reg_i = value_i; /* register has dp value of 0.666for next stage2 for next stage */
                        *reg_i_lsw = 0x0;
                        *(u_long *)FPA_IMASK_PTR = 0x1;

                        *reg0_addr = nxt_cmd[0].data; /* nota number + 0 weitek op */

                        *reg_i_addr = nxt_cmd[i].data; /* nota number + 0 weitek op */

                        /* may be soft clear */  
                        shad_res_msw = *shadow_j; /* read the shadow register */
                        shad_res_lsw = *shadow_j_lsw;

                        *soft_clear = 0x0;

                        res_i_msw = *reg_i; /* read the result from the reg  i*/
                        res_i_lsw = *reg_i_lsw;

                        res0_msw = *reg0;
                        res0_lsw = *reg0_lsw;
                        *(u_long *)FPA_IMASK_PTR = 0x0;

                        if ((j == 0) && (j == i))
                        {
                        if ((shad_res_msw != value_i)||(res_i_msw != value_i)||(res0_msw != value_i))

/*      if ((shad_res_msw != res_n_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_n_msw)) */
                        {
                         
                                 /*       printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
                                                i, j, res0_msw, shad_res_msw);                                        printf("res0_lsw = %x, shad_lsw = %x, resi_msw = %x, resi_lsw = %x\n",
                                                res0_lsw, shad_res_lsw, res_i_msw, res_i_lsw);
                                  */
			         return(-1); 
                        }
                        }

                        else                       if ((j == 0) && (j != i))
                        {
                if ((shad_res_msw != res_a_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw))
                        {
                         
                          /*              printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n", 
                                                i, j, res0_msw, shad_res_msw);                 
                       		        printf("res_i_msw = %x\n", res_i_msw); 
                           */
			            return(-1); 
                        }
                        }
                        else if((j != 0) && (j == i)) {
                                 
                if ((shad_res_msw != res_n_msw) || (res_i_msw != res_n_msw) || (res0_msw != res_a_msw))
                        {
                           /*   printf("Err3:reg = %x, shadow = %x, rres= %x, sres = %x\n",
                                    i, j, res0_msw, shad_res_msw);
                        	printf("res_i_msw = %x\n", res_i_msw);
                            */
			            return(-1);
                        }
                        }
                        else if ((j != 0) && (j != i)) {
                                if (i == 0) {
                if ((shad_res_msw != 0x0)|| (res_i_msw != value_i) || (res0_msw != value_i)) {
                          /*   printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",
                                i, j, res0_msw, shad_res_msw);
        			printf("res_i_msw = %x\n", res_i_msw);
                           */
		                return(-1);  
                        }
                        }
                else
                         
                if ((shad_res_msw != 0x0)|| (res_i_msw != value_i) || (res0_msw != value_0))
                        {
                         /*   printf("Err4:reg = %x, shadow = %x, rres= %x, sres = %x\n",    
                                    i, j, res0_msw, shad_res_msw); 
        			printf("res_i_msw = %x\n", res_i_msw);
                          */
		              return(-1);    
                        }      
                        }        
                        *reg0 = 0x0;
                        *reg0_lsw = 0x0;
                        *reg_i = 0x0;
                        *reg_i_lsw = 0x0;
                }
                *ptr2 = 0x0;
                *ptr2_lsw = 0x0;
        }
        return(0);

}

dp_short_test()
{
	u_long	res, res1_lsw, res1_msw, res2_lsw, res2_msw, res3_lsw, res3_msw;
	u_long	i, j;
	u_long	*soft_clear, *ptr1, *ptr2, *ptr1_lsw, *ptr2_lsw, *ptr3, *ptr3_lsw;
	u_long *pipe;

	pipe = (u_long *)(FPA_BASE + FPA_STABLE_PIPE_STATUS);	
	soft_clear = (u_long *)FPA_CLEAR_PIPE_PTR;
	for (i = 0; i < 16; i++)
	{
		ptr2 = (u_long *)user[i].reg_msw;  
                ptr2_lsw = (u_long *)user[i].reg_lsw; 
		
		/* initialize */
		*ptr2 = 0x0;
		*ptr2_lsw = 0x0;
	}
	res3_msw = 0x3FD55555; /* the result is always dp value 0.3333333333333333 */
	res3_lsw = 0x55555555;
	for (i = 0; i < 8; i++)
	{
                 ptr1 = (u_long *)dps[i].addr; /* for higher significant value */ 
                 ptr1_lsw = (u_long *)0xE0001000;  /* for least significant value */     
                 ptr2 = (u_long *)user[i].reg_msw; 
                 ptr2_lsw = (u_long *)user[i].reg_lsw;
		for (j = 0; j < 8; j++)
		{

			ptr3 = (u_long *)shadow[j].shreg_msw;
			ptr3_lsw = (u_long *)shadow[j].shreg_lsw;
	                *ptr2 = 0x3FF00000; /* register has dp value 1 */
	                *ptr2_lsw = 0x0;
		        *(u_long *)FPA_IMASK_PTR = 0x1;

	                *ptr1 = 0x40080000; /* operand is a dp value  3 */ 

	                *ptr1_lsw = 0x0;
			res1_msw = *ptr3; /* read the shadow register */
			res1_lsw = *ptr3_lsw;
			*soft_clear = 0x0; 
			res2_msw = *ptr2; /* read the result from the reg */
			res2_lsw = *ptr2_lsw;
			*(u_long *)FPA_IMASK_PTR = 0x0;
			if (i == j) {

				if ((res3_msw != res2_msw) || (res3_msw != res1_msw))
				{
			/*		printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
						i, j, res2_msw, res1_msw);
			*/
					return(-1); 
				}
			}
			if (i != j)	
			{
				if ((res1_msw != 0x0) || (res2_msw != 0x3FF00000)) {
			/*		printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n",                                                 i, j, res2_msw, res1_msw);
			 */
					return(-1); 
				}
			}

		}
		*ptr2 = 0x0;
		*ptr2_lsw = 0x0;
	}
	return(0);
}			

dp_ext_test()
{
	u_long  res, res1_lsw, res1_msw, res2_lsw, res2_msw, res3_lsw, res3_msw;
        int     i, j;
        u_long  *soft_clear, *ptr1, *ptr2, *ptr1_lsw, *ptr2_lsw, *ptr3, *ptr3_lsw;
         
        soft_clear = (u_long *)FPA_CLEAR_PIPE_PTR;
        for (i = 0; i < 16; i++)
        {
                ptr2 = (u_long *)user[i].reg_msw;  
                ptr2_lsw = (u_long *)user[i].reg_lsw; 
                 
                /* initialize */
                *ptr2 = 0x0;
                *ptr2_lsw = 0x0;
        }
        res3_msw = 0x3FD55555; /* the result is always dp value 0.33333333 */
        res3_lsw = 0x55555555;

        for (i = 0; i < 16; i++)
        {
		ptr1 = (u_long *)ext[i].addr;
		ptr1_lsw = (u_long *)ext[i].addr_lsw;
                ptr2 = (u_long *)user[i].reg_msw; 
                ptr2_lsw = (u_long *)user[i].reg_lsw;
                for (j = 0; j < 8; j++)
                {
                        ptr3 = (u_long *)shadow[j].shreg_msw;
                        ptr3_lsw = (u_long *)shadow[j].shreg_lsw;
                        *ptr2 = 0x3FF00000; /* register has dp value 1 */
                        *ptr2_lsw = 0x0;
			*(u_long *)FPA_IMASK_PTR = 0x1;

                        *ptr1 = 0x40080000; /* operand is a dp value 3 */ 
                        *ptr1_lsw = 0x0;
                        res1_msw = *ptr3; /* read the shadow register */
                        res1_lsw = *ptr3_lsw;
                        *soft_clear = 0x0;

                        res2_msw = *ptr2; /* read the result from the reg */
                        res2_lsw = *ptr2_lsw;
			*(u_long *)FPA_IMASK_PTR = 0x2;
			
			if (i == j) {
                        	if ((res3_msw != res2_msw) && (res3_msw != res1_msw))
                        	{
                                    /*    printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
                                                i, j, res2_msw, res1_msw);
                                     */
					   return(-1);
                                }
                        }
                        else  { 
				if ((res1_msw != 0x0) || (res2_msw != 0x3FF00000)){
                                 /*       printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n",                                                 i, j, res2_msw, res1_msw);
                                  */
				      return(-1);
                                }
                        }

                }
                *ptr2 = 0x0;
                *ptr2_lsw = 0x0;
        }
        return(0);
}                        

dp_cmd_test()
{
        u_long  res, res1_lsw, res1_msw, res2_lsw, res2_msw, res3_lsw, res3_msw;
        int     i, j, k, l;
        u_long  *soft_clear, *ptr1, *ptr2, *ptr1_lsw, *ptr2_lsw, *ptr3, *ptr3_lsw;
        u_long  *ptr4, *ptr4_lsw, *ptr5, *ptr5_lsw;
 
        soft_clear = (u_long *)FPA_CLEAR_PIPE_PTR;
        for (i = 0; i < 32; i++)
        {
                ptr2 = (u_long *)user[i].reg_msw;  
                ptr2_lsw = (u_long *)user[i].reg_lsw; 
                 
                /* initialize */
                *ptr2 = 0x0;
                *ptr2_lsw = 0x0;
        }
        res3_msw = 0x3FD55555; /* the result is always dp value 0.3333333333 */
        res3_lsw = 0x55555555;

        for (i = 0; i < 30; i++)
        {
                 ptr1 = (u_long *)0xE0000A34; /* for dp divide from weitek spec */ 
                 ptr2 = (u_long *)user[i].reg_msw; 
                 ptr2_lsw = (u_long *)user[i].reg_lsw;
		k = i + 1;
		l = i + 2;
		ptr4 = (u_long *)user[k].reg_msw;
		ptr4_lsw = (u_long *)user[k].reg_lsw;
		ptr5 = (u_long *)user[l].reg_msw;
		ptr5_lsw = (u_long *)user[l].reg_lsw;
		
                for (j = 0; j < 8; j++)
                {
                        ptr3 = (u_long *)shadow[j].shreg_msw;
                        ptr3_lsw = (u_long *)shadow[j].shreg_lsw;
                        *ptr4 = 0x3FF00000; /* register has dp value 1 */
			*ptr4_lsw = 0x0;
			*ptr5 = 0x40080000; /* register has dp value 3 */
			*ptr5_lsw = 0x0;

			*(u_long *)FPA_IMASK_PTR = 0x1;

                        *ptr1 = cmd[i].data; 
                        res1_msw = *ptr3; /* read the shadow register */
                        res1_lsw = *ptr3_lsw;
                        *soft_clear = 0x0;

                        res2_msw = *ptr2; /* read the result from the reg */
                        res2_lsw = *ptr2_lsw;
			*(u_long *)FPA_IMASK_PTR = 0x0;
			
			if (i == j) {

	                        if ((res3_msw != res2_msw) || (res3_msw != res1_msw))
       		                {
                                     /*   printf("Err1:reg = %x, shadow = %x, rres= %x, sres = %x\n",
                                                i, j, res2_msw, res1_msw);
                                      */
				    return(-1);
                                }
                        }
                        else if (j == k) {
					if ((res2_msw != 0x0) || (res1_msw != 0x3FF00000)) {
				/*		printf("Err2:reg = %x, shadow = %x, rres= %x, sres = %x\n",
       			                             i, j, res2_msw, res1_msw);
                       		 */
			                return(-1);
                               		 }
			}
			else if (j == l) {
					if ((res2_msw != 0x0) || (res1_msw != 0x40080000)) { 
                                /*                printf("Err3:reg = %x, shadow = %x, rres= %x, sres = %x\n", 
                                                     i, j, res2_msw, res1_msw); 
                                 */
			                return(-1); 
                                         } 
                        } 
			*ptr2 = 0x0;
			*ptr2_lsw = 0x0;
                }
        }
        return(0);
}                        



