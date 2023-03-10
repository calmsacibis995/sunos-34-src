/*
 *      static char     fpasccsid[] = "@(#)pointers.c 1.1 9/25/86 Sun right Sun Microsystems";
 *
 *      Copyright (c) 1985 by Sun Microsystems, Inc 
 *
 */

#include <sys/types.h>
#include "fpa.h"

extern u_long users[];


u_long sp_short[] =
{
	0xE0000380,
	0xE0000388,
	0xE0000390,
	0xE0000398,
        0xE00003A0, 
        0xE00003A8, 
        0xE00003B0, 
        0xE00003B8, 
        0xE00003C0, 
        0xE00003C8, 
        0xE00003D0, 
        0xE00003D8, 
        0xE00003E0, 
        0xE00003E8, 
        0xE00003F0, 
        0xE00003F8 
};
u_long dp_short[] = 
{
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
struct single_ext {
	u_long high;
	u_long low;
};
struct single_ext dp_extd[] =
{
        0xE000180C, 0xE0001900,
        0xE0001814, 0xE0001988,
        0xE000181C, 0xE0001A10,
        0xE0001824, 0xE0001A98,
        0xE000182C, 0xE0001B20,
        0xE0001834, 0xE0001BA8,
        0xE000183C, 0xE0001C30,
        0xE0001844, 0xE0001CB8,
        0xE000184C, 0xE0001D40,
        0xE0001854, 0xE0001DC8,
        0xE000185C, 0xE0001E50,
        0xE0001864, 0xE0001ED8,
        0xE000186C, 0xE0001F60,
        0xE0001874, 0xE0001FE8,
        0xE000187C, 0xE0001870,
	0xE0001804, 0xE00018F8
};
struct single_ext sp_extd[] =
{
	0xE0001008, 0xE0001800,
        0xE0001010, 0xE0001808, 
        0xE0001018, 0xE0001810, 
        0xE0001020, 0xE0001818, 
        0xE0001028, 0xE0001820, 
        0xE0001030, 0xE0001828, 
        0xE0001038, 0xE0001830, 
        0xE0001040, 0xE0001838, 
        0xE0001048, 0xE0001840, 
        0xE0001050, 0xE0001848, 
        0xE0001058, 0xE0001850, 
        0xE0001060, 0xE0001858, 
        0xE0001068, 0xE0001860, 
        0xE0001070, 0xE0001868, 
        0xE0001078, 0xE0001870, 
        0xE0001000, 0xE0001878 
};
u_long cmd_reg[] =
{
	0x0C020040,
	0x10030081,
	0x140400C2,
	0x18050103,
	0x1C060144,
	0x20070185,
	0x240801C6,
	0x28090207,
	0x2C0A0248,
	0x300B0289,
	0x340C02CA,
	0x380D030B,
	0x3C0E034C,
	0x400F038D,
	0x441003CE,
	0x4811040F,
	0x4C120450,
	0x50130491,
	0x541404D2,
	0x58150513,
	0x5C160554,
	0x60170595,
	0x641805D6,
	0x68190617,
	0x6C1A0658,
	0x701B0699,
	0x741C06DA,
	0x781D071B,
	0x7C1E075C
};
struct sp_dp_cmd
{
        u_long reg1;
        u_long reg2;
        u_long reg3;
        u_long res;
};
struct sp_dp_cmd sp_dp_res[] =
{
        0x40000000, 0x41000000, 0x40800000, 0x41800000,
        0x40000000, 0x40200000, 0x40100000, 0x40300000
};


		
pointer_test()
{
	if (pointer_short())
		return(-1);
	if (pointer_sp_ext())
		return(-1);
	if (pointer_dp_ext())
		return(-1);
	if (pointer_cmd())
		return(-1);
	return(0);
}
	

pointer_cmd()
{
        u_long  temp_ptr3, temp_ptr4, res1, res2 ;
        u_long  *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
        u_char  i,j,k,l,m,n;

        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;

	for (n = 0; n < 2; n++)
	{
		if (n == 0)
			ptr1 = (u_long *)0xE0000888;
		else 
			ptr1 = (u_long *)0xE000088C;
		
	
		for (i = 0; i < 28; i++)
		{
			for (j = 0; j < 32; j++)
			{
				ptr2 = (u_long *)users[j];
               		        *ptr2 = 0x0;
				ptr2++;
				*ptr2 = 0x0; /* low order word */
			}
			k = i+1;
			l = i+2;
			m = i+3;
			
			ptr2 = (u_long *)users[k];
			ptr3 = (u_long *)users[l];
			ptr4 = (u_long *)users[m];
		
			*ptr2 = sp_dp_res[n].reg1;
			*ptr3 = sp_dp_res[n].reg2;
			*ptr4 = sp_dp_res[n].reg3;

			*ptr1 = cmd_reg[i];

			for (j = 0; j < 32; j++)
			{
				ptr5 = (u_long *)users[j];
				res1 = *ptr5;
		
				if (i == j) {
					if (res1 != sp_dp_res[n].res) 
						return(-1);
				}
				else	
				if (j == k) {
					if (res1 != sp_dp_res[n].reg1) 
						return(-1);
               		         }       
				else
				if (j == l) {
					if (res1 != sp_dp_res[n].reg2) 
						return(-1); 
                       		 }        
				else if (j == m) {
					if (res1 != sp_dp_res[n].reg3) 
						return(-1);
                        	}         
				else
					if (res1 != 0x0) 
                        			return(-1);
			}
		}
	}
	return(0);
}
pointer_sp_ext()
{
        u_long  temp_ptr3, temp_ptr4, res1, res2;
        u_long  *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
        u_char  i,j,k,l,m;
 
        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;
 
        for (i = 0; i < 16; i++)
        {

                ptr4 = (u_long *)sp_extd[i].high;
                ptr5 = (u_long *)sp_extd[i].low;
                for (j = 0; j < 16; j++)
                {
                        ptr1 = (u_long *)users[j];
                        *ptr1 = 0x0;
			ptr1++;
			*ptr1 = 0x0;
                }
                k = (i+1) & 0xF;
		ptr2 = (u_long *)users[k]; 
		*ptr2 = 0x40000000; /* sp 2 */
		
		*ptr4 = 0x40400000; /* operand1 sp 3 */
		*ptr5 = 0x40800000; /* operand2 sp 4 */

		for (j = 0; j < 16; j++)
                {

                        ptr1 = (u_long *)users[j];
                        res1 = *ptr1;
                        if (i == j) { /*  it should have the result */
                                if (res1 != 0x41200000) 
                        		return(-1);
			}
                        else     
                        if (j == k) { /* reg1 + 1 should have value sp 2 */
                                if (res1 != 0x40000000) 
                        	return(-1);
			}
                        else
                                if (res1 != 0x0) 
                			return(-1);
		}
        }
        return(0);
}        




pointer_dp_ext()
{
        u_long  temp_ptr3, temp_ptr4, res1, res2;
        u_long  *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;
	u_char  i,j,k,l,m;

	/* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;
 
	for (i = 0; i < 16; i++)
	{
		
		ptr4 = (u_long *)dp_extd[i].high;
		ptr5 = (u_long *)dp_extd[i].low;
		for (j = 0; j < 16; j++)
		{
			ptr1 = (u_long *)users[j];
                        *ptr1 = 0x0;
			ptr1++;
			*ptr1 = 0x0;
                }
		/* the instru ction is reg[i] = reg[i+2] + (reg[i+1] * operand ) */
		k = (i+1) & 0xF;
		l = (i+2) & 0xF;

		ptr2 = (u_long *)users[k];  
		ptr3 = (u_long *)users[l];
		*ptr2 = 0x40000000; /* sp 2 , reg[i+1] */
		*ptr3 = 0x40080000; /* sp 3 ,  reg[i+2] */
		
		*ptr4 = 0x40080000; /* operand sp 3 */
		*ptr5 = 0x0;

		for (j = 0; j < 16; j++)
		{

			ptr1 = (u_long *)users[j];
			res1 = *ptr1;
			if (i == j) { /*  it should have the result */
				if (res1 != 0x40220000) 
					return(-1);
				
			}
			else	
			if (k == j) { /* reg1 + 1 should have value sp 2 */
				if (res1 != 0x40000000) 
                        		return(-1);
			}
			else	
			if (l == j) { /* reg1 + 2 should have value sp 3 */
				if (res1 != 0x40080000) 
                        	return(-1);
			}      
			else
				if (res1 != 0x0) 
					return(-1);
		}
	}
	return(0);
}	
 




pointer_short()
{
	u_long	i, j, k, l, m, n, temp_ptr3, temp_ptr4, res1, res2;
	u_long  *ptr1, *ptr2, *ptr3, *ptr4;

	
        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;



	for (i = 0; i < 16; i++)
	{
		ptr3 = (u_long *)sp_short[i];

		for (k = 0; k < 16; k++)
		{
			ptr1 = (u_long *)users[k];
			*ptr1 = 0x0;
			ptr1++;
			*ptr1 = 0x0;
		}
		ptr1 = (u_long *)users[i];
		*ptr1 = 0x40000000; /* 2 */
		*ptr3 = 0x40C00000; /* sp 6 */

		for (k = 0; k < 16; k++)
		{
			ptr1 = (u_long *)users[k];
			res1 = *ptr1;
			if (i == k)
			{
				if (res1 != 0x41000000)
					return(-1); 
			}
			else
			{
				if (res1 != 0x0)
                                        return(-1); 
			}
		}
	}
	for (i = 0; i < 16; i++)
        {
                ptr3 = (u_long *)dp_short[i];
		ptr4 = (u_long *)0xE0001000;
 
                for (k = 0; k < 16; k++)
                {
                        ptr1 = (u_long *)users[k];
                        *ptr1 = 0x0;
			ptr1++;
			*ptr1 = 0x0;
                }
                ptr1 = (u_long *)users[i];
                *ptr1 = 0x40000000; /* dp 2 */
                *ptr3 = 0x40180000; /* dp 6 */
		*ptr4 = 0x0;
 
                for (k = 0; k < 16; k++)
                {
                        ptr1 = (u_long *)users[k];
                        res1 = *ptr1;
                        if (i == k)
                        {
                                if (res1 != 0x40200000)
                                         return(-1); 
                        }
                        else
                        {
                                if (res1 != 0x0)
                                        return(-1); 
                        }
                }
        }

	return(0);
}
