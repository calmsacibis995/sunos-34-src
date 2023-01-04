/*
 *      static char     fpasccsid[] = "@(#)ptr_incdec.c 1.1 2/14/86 Copyright Sun Microsystems";
 *
 *      Copyright (c) 1985 by Sun Microsystems, Inc 
 *
 */

#include <sys/types.h>
#include "fpa.h"

struct regs
{
        u_long  reg;
};

struct regs users[] = {

        0xE0000C00,
        0xE0000C08, 
        0xE0000C10,  
        0xE0000C18,  
        0xE0000C20,   
        0xE0000C28,    
        0xE0000C30,  
        0xE0000C38, 
        0xE0000C40,
        0xE0000C48,    
        0xE0000C50,   
        0xE0000C58,  
        0xE0000C60, 
        0xE0000C68,
        0xE0000C70,    
        0xE0000C78,   
        0xE0000C80,  
        0xE0000C88,    
        0xE0000C90,    
        0xE0000C98,    
        0xE0000CA0,    
        0xE0000CA8,    
        0xE0000CB0,    
        0xE0000CB8,    
        0xE0000CC0,    
        0xE0000CC8,    
        0xE0000CD0,    
        0xE0000CD8,    
        0xE0000CE0,    
        0xE0000CE8,    
        0xE0000CF0,    
        0xE0000CF8    
};
struct ptr_command
{
	u_long data;
};
struct ptr_command ptr_cmd[] =
{
	0x10005,
	0x20046,
	0x30087,
	0x400C8,
	0x50109,
	0x6014A,
	0x7018B,
	0x801CC,
	0x9020D,
	0xA024E,
	0xB028F,
	0xC02D0,
	0xD0311,
	0xE0352,
	0xF0393,
	0x1003D4,
	0x110415,
	0x120456,
	0x130497,
	0x1404D8,
	0x150519,
	0x16055A,
	0x17059B,
	0x1805DC,
	0x19061D,
	0x1A065E,
	0x1B069F
};
u_long val[] = {

	0x3FF00000, /* for dp 1 */
	0x40000000, /* for dp 2 */
	0x40080000, /* for dp 3 */
	0x40100000, /* for dp 4 */
	0x40140000  /* for dp 5 */
};

ptr_incdec_test()
{
	u_long	i, j, k, l,m,n, res1, res2;
	u_long  *ptr, *ptr2, *ptr3;


        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;


	ptr = (u_long *)0xE00009B0; /* for transposing */

	for (i = 0; i < 1; i++)
	{
		for (j = 0; j <= 15; j++)
		{
			ptr2 = (u_long *)users[i+j].reg;
			*ptr2 = j;
			
			k = (i+j+16) & 0x1F; /* so that the register will be 0 - 31 */	
			ptr3 = (u_long *)users[k].reg;
			*ptr3 = 0x100;
		}			
		*ptr = i;
		/* now read the transposed values */
		j = 0;
		for (m = 0; m < 4; m++)
		{
			for (n = 0; n < 4; n++)
			{
				ptr2 = (u_long *)users[i+j].reg;
	                        k = (i+j+16) & 0x1F; /* so that the register will be 0 - 31 */
                        	ptr3 = (u_long *)users[k].reg;
				res1 =  m + (n*4);

				if (*ptr2 != res1) 
					return(-1);
				
				if (*ptr3 != 0x100) 
					return(-1);
				
				j++;
			}
		}
	}

	        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;

	ptr = (u_long *)0xE00008CC; /* for dot product */

	for (i = 0; i < 32; i++)
	{
		ptr2 = (u_long *)users[i].reg;
		*ptr2 = 0x0;
	}
	for (i = 0; i <= 26; i++)
	{
		ptr3 = (u_long *)users[i+5].reg;

		for (j = 0; j < 32; j++) 
	        {
       		         ptr2 = (u_long *)users[j].reg;
               		 *ptr2 = 0x0;
			 ptr2++;
			*ptr2 = 0x0;
        	}

		for (j = 0; j <= 4; j++)
		{
	                ptr2 = (u_long *)users[i+j].reg;
			*ptr2 = val[j];
		}
		
		*ptr = ptr_cmd[i].data; /* send the insrtuction */
		res1 = *ptr3;  /* get the result */
		if (res1 != 0x40440000) 
			  return(-1);  
		
	}
	return(0);
}
