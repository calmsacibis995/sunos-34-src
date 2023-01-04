#include <sys/types.h>
#include <machdep.h>
#include <exception.h>

static char	sccsid[] = "@(#)exception.c 1.1 86/09/25 Copyright Sun Micro";

exception(info)
struct	ex_info		info;
{
	berr_size	berr;

	if (exception_print){

		printf("exception 0x%x @ 0x%x sr 0x%x (fmt %x)\n",
			info.e_offset, info.e_pc, info.e_sr, info.e_format);

		if (info.e_format == LONG_FORMAT){
			berr = getberrreg() & 0xff;
			printf("fault @ 0x%x buserr reg 0x%x\n",
				info.e_fault, berr);
			printf("rr %d if %d df %d rm %d hb %d by %d rw %d fc %d\n",
				info.e_rr, info.e_if, info.e_df, info.e_rm, 
				info.e_hb, info.e_by, info.e_rw, info.e_fc); 
			printf("buffers: datain 0x%x dataout 0x%x inst 0x%x\n",
				info.e_datain, info.e_dataout, info.e_inst); 
		}
	}
	if (exception_handler)
		(*exception_handler)(&info);
}
