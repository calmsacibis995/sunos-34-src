
/*
 * static char     fpamfsccsid[] = "@(#)contexts.c 1.1 9/25/86 Copyright Sun Microsystems";
  */


#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include "fpa.h"
#include <sys/ioctl.h>
#include </usr/include/sundev/fpareg.h>

extern int errno;
extern u_long  contexts;
extern int reg_ram();

int	dev_no;
extern	int verbose_mode;

/*
 * This routine opens the FPA context, and if it could not checks the
 * error no and prints the error
 */

open_new_context()
{
	u_long	value, *st_reg_ptr;

	st_reg_ptr = (u_long *)FPA_STATE_PTR;
	dev_no = open("/dev/fpa", O_RDWR);
	if (dev_no < 0) {	
 		/* could not open */

		switch (errno) {

			case ENXIO:
				if (verbose_mode)
					printf("Cannot find FPA.\n");
				break;
			case ENOENT:
				if (verbose_mode)
					printf("Cannot find 68881.\n");
				break;
			case EBUSY:
				if (verbose_mode)
					printf("No FPA context Available.\n");
				break;
			case ENETDOWN:
				if (verbose_mode)
					printf("Disabled FPA, could not access.\n");
				break;
			case EEXIST:
				if (verbose_mode)
					printf("Duplicate Open on FPA.\n");
				break;
		}
		return(errno);
	}
	else 
		return(0);
}

close_new_context()
{
	int	val;

	if ((val = close(dev_no)) < 0) { 
		printf("Error while closing a context number.\n");
		printf("	descriptor number = %x, error number = %d.\n", dev_no, errno);
	}
}

other_contexts()
{
	int	i, j; 
	u_long	val, val1;
	u_long	*st_reg_ptr;
	u_char  ret_val;
	int	value;
	char    null_str[2];

/*  the return is 
 *  0 - success
 *  0xff - the test failed
 *  other positive number - could not open fpa
 *
 */
	st_reg_ptr = (u_long *)FPA_STATE_PTR;
	for (i = 1; i < 32; i++) {

		if (ret_val = open_new_context())
				return(ret_val);

		val = *st_reg_ptr & 0x1F; /* get the context number */
		val1 = (1 << val);
		if (verbose_mode)  
                        printf("        Register Ram Lower Half Test, Context Number = %d\n",val);

		contexts = (contexts | val1);
	
		if (reg_ram()) 
			return(0xff);
		close_new_context();
	}
	return(0);
}

			
		
