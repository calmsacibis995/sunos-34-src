#ifdef sccsid
static	char sccsid[] = "@(#)winitfp.c 1.1 86/09/24 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include <sys/file.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include "fpcrttypes.h"

extern int errno;
       int _fpa_fd ;		/* FPA file descriptor - needed outside at times. */

int sigfpe_handler( sig, code, scp)
int sig, code ;
struct sigcontext *scp ;

{

if (code == FPE_FPA_ERROR) 
	{
	if(fpa_handler(scp)) return ;
	}
else
	{
	fprintf(stderr,"\n SIGFPE Handler pc %X ",scp->sc_pc) ;
	switch(code)
	{
	case FPE_INTDIV_TRAP: fprintf(stderr," INTDIV_TRAP ") ; break ;
	case FPE_CHKINST_TRAP: fprintf(stderr," CHKINST_TRAP ") ; break ;
	case FPE_TRAPV_TRAP: fprintf(stderr," TRAPV_TRAP ") ; break ;
	case FPE_FLTBSUN_TRAP: fprintf(stderr," FLTBSUN_TRAP ") ; break ;
	case FPE_FLTINEX_TRAP: fprintf(stderr," FLTINEX_TRAP ") ; break ;
	case FPE_FLTDIV_TRAP: fprintf(stderr," FLTDIV_TRAP ") ; break ;
	case FPE_FLTUND_TRAP: fprintf(stderr," FLTUND_TRAP ") ; break ;
	case FPE_FLTOPERR_TRAP: fprintf(stderr," FLTOPERR_TRAP ") ; break ;
	case FPE_FLTOVF_TRAP: fprintf(stderr," FLTOVF_TRAP ") ; break ;
	case FPE_FLTNAN_TRAP: fprintf(stderr," FLTNAN_TRAP ") ; break ;
	case FPE_FPA_ENABLE:  fprintf(stderr," FPA_ENABLE ") ; break ;
	case FPE_FPA_ERROR:  fprintf(stderr," FPA_ERROR ") ; break ;
	default: fprintf(stderr," code %X ",code) ;
	}
	fprintf(stderr,"\n") ; fflush(stderr) ;
	}
abort() ;
}

int winitfp_()

/*
 *	Procedure to determine if a physical FPA and 68881 are present and
 *	set fp_state_sunfpa and fp_state_mc68881 accordingly.
	Also returns 1 if both present, 0 otherwise.
 */

{
int mode81 ;
struct sigvec newfpe, oldfpe ;
long *fpaptr ;

if (fp_state_sunfpa == fp_unknown) 
	{
	if (minitfp_() != 1) fp_state_sunfpa = fp_absent ;
	else
		{
		_fpa_fd = open("/dev/fpa", O_RDWR);
		if ((_fpa_fd < 0) && (errno != EEXIST)) 
			{ /* _fpa_fd < 0 */
			if (errno == EBUSY) 
				{
				fprintf(stderr,"\n No Sun FPA contexts available - all in use\n"); fflush(stderr) ;
				}
			fp_state_sunfpa = fp_absent ;
			} /* _fpa_fd < 0 */
		else
			{ /* _fpa_fd >= 0 */
			if (errno == EEXIST) 
				{
				fprintf(stderr,"\n  FPA was already open \n") ; fflush(stderr) ;
				}
			/* to close FPA context on execve() */
			fcntl(_fpa_fd, F_SETFD, 1);
			newfpe.sv_mask = 0 ;
			newfpe.sv_onstack = 0 ;
			newfpe.sv_handler = sigfpe_handler ;
			sigvec( SIGFPE, &newfpe, &oldfpe ) ;
			fpaptr = (int*) 0xe00008d0 ; /* write mode register */
			*fpaptr = 2 ; /* set to round integers toward zero */
			fpaptr = (int*) 0xe0000f14 ;
			*fpaptr = 1 ; /* set imask to one */
			fp_state_sunfpa = fp_enabled ;
			if (!MA93N()) 
			{
			fprintf(stderr,"\n Warning! Sun FPA works best with 68881 mask A93N \n") ;
			fflush(stderr) ;
                	}
			} /* _fpa_fd >= 0 */
		}
	}
if (fp_state_sunfpa == fp_enabled) fp_switch = fp_sunfpa ;
return((fp_state_sunfpa == fp_enabled) ? 1 : 0) ;
}
