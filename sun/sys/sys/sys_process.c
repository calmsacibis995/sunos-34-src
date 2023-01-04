/*	@(#)sys_process.c 1.1 86/09/25 SMI; from UCB 5.10 83/07/01	*/

#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/text.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/acct.h"
#include "../h/ptrace.h"

#ifdef sun
#include "fpa.h"
#if NFPA > 0
#include "../sundev/fpareg.h"
#endif NFPA > 0
#endif sun

/*
 * Priority for tracing
 */
#define	IPCPRI	PZERO

#ifdef vax
#define	NIPCREG 16
#endif
#ifdef mc68000
#define	NIPCREG 17
#endif
int ipcreg[NIPCREG] =
#ifdef vax
	{R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,AP,FP,SP,PC};
#endif
#ifdef mc68000
	{R0,R1,R2,R3,R4,R5,R6,R7,AR0,AR1,AR2,AR3,AR4,AR5,AR6,AR7,PC};
#endif

#define	PHYSOFF(p, o) \
	((physadr)(p)+((o)/sizeof (((physadr)0)->r[0])))

/*
 * Tracing variables.
 * Used to pass trace command from
 * parent to child being traced.
 * This data base cannot be
 * shared and is locked
 * per user.
 */
struct {
	int	ip_lock;		/* Locking between processes */
	short	ip_error;		/* The child encountered an error? */
	enum	ptracereq ip_req;	/* The type of request */
	caddr_t	ip_addr;		/* The address in the child */
	caddr_t	ip_addr2;		/* The address in the parent */
	int	ip_data;		/* Data or number of bytes */
	struct	regs ip_regs;		/* The regs, psw and pc */
	int	ip_nbytes;		/* # of bytes being moved in bigbuf */
	caddr_t	ip_bigbuf;		/* Dynamic buffer for large transfers */
#ifdef sun3
	struct	fp_status ip_fp_status;	/* Floating point processor info */
#if NFPA > 0
	struct fpa_regs ip_fpa_regs;	/* FPA processor regs */
#define ip_fpa_flags	ip_fpa_regs.fpar_flags
#define ip_fpa_state	ip_fpa_regs.fpar_status.fpas_state
#define ip_fpa_status	ip_fpa_regs.fpar_status
#define ip_fpa_data	ip_fpa_regs.fpar_data
#endif NFPA > 0
#endif sun3
} ipc;


/*
 * sys-trace system call.
 */
ptrace()
{
	register struct proc *p;
	register struct a {
		enum ptracereq req;
		int	pid;
		caddr_t	addr;
		int	data;
		caddr_t addr2;
	} *uap;

	uap = (struct a *)u.u_ap;
	if (uap->req == PTRACE_TRACEME) {
		u.u_procp->p_flag |= STRC;
		u.u_procp->p_tptr = u.u_procp->p_pptr;
		return;
	}
	p = pfind(uap->pid);
	if (p == NULL) {
		u.u_error = ESRCH;
		return;
	}
	if (uap->req == PTRACE_ATTACH) {
		if ((u.u_uid && u.u_uid != p->p_uid) || (p->p_flag & STRC)) {
			u.u_error = EPERM;
			return;
		}
		/*
		 * Tell him we're tracing him and stop him where he is.
		 */
		p->p_flag |= STRC;
		p->p_tptr = u.u_procp;
		psignal(p, SIGSTOP);
		return;
	}
	if (p->p_stat != SSTOP || p->p_tptr != u.u_procp ||
	    !(p->p_flag & STRC)) {
		u.u_error = ESRCH;
		return;
	}
	while (ipc.ip_lock)
		(void) sleep((caddr_t)&ipc, IPCPRI);
	ipc.ip_lock = p->p_pid;
	ipc.ip_error = 0;
	ipc.ip_data = uap->data;
	ipc.ip_addr = uap->addr;
	ipc.ip_req = uap->req;
	ipc.ip_addr2 = uap->addr2;
	p->p_flag &= ~SWTED;
	u.u_r.r_val1 = 0;

	switch (uap->req) {
	case PTRACE_GETREGS:
		runchild(p);
		if (copyout((caddr_t)&ipc.ip_regs, ipc.ip_addr, 
		    sizeof (ipc.ip_regs)) != 0) {
			ipc.ip_error = 1;
		}
		break;

	case PTRACE_SETREGS:
		if (copyin(ipc.ip_addr, (caddr_t)&ipc.ip_regs, 
		    sizeof (ipc.ip_regs)) != 0) {
			ipc.ip_error = 1;
		}
		runchild(p);
		break;

#ifdef sun3
	case PTRACE_GETFPREGS:
		runchild(p);
		if (copyout((caddr_t)&ipc.ip_fp_status, ipc.ip_addr, 
		    sizeof (ipc.ip_fp_status)) != 0) {
			ipc.ip_error = 1;
		}
		break;

	case PTRACE_SETFPREGS:
		if (copyin(ipc.ip_addr, (caddr_t)&ipc.ip_fp_status, 
		    sizeof (ipc.ip_fp_status)) != 0) {
			ipc.ip_error = 1;
		}
		runchild(p);
		break;
#if NFPA > 0
	case PTRACE_GETFPAREGS:
		runchild(p);
		if (copyout((caddr_t)&ipc.ip_fpa_regs, ipc.ip_addr,
		    sizeof (ipc.ip_fpa_regs)) != 0) {
			ipc.ip_error = 1;
		}
		break;
	
	case PTRACE_SETFPAREGS:
		if (copyin(ipc.ip_addr, (caddr_t)&ipc.ip_fpa_regs,
		    sizeof (ipc.ip_fpa_regs)) != 0) {
			ipc.ip_error = 1;
		}
		runchild(p);
		break;
#endif NFPA > 0
#endif sun3

	case PTRACE_READDATA:
	case PTRACE_READTEXT:
		ipc.ip_bigbuf = (caddr_t)kmem_alloc(CLBYTES);
		while (ipc.ip_data > 0) {
			ipc.ip_req = uap->req;
			ipc.ip_nbytes = MIN(ipc.ip_data, CLBYTES);
			runchild(p);
			if (copyout(ipc.ip_bigbuf, ipc.ip_addr2, 
			    (u_int)ipc.ip_nbytes) != 0) {
				ipc.ip_error = 1;
				break;
			}
			ipc.ip_addr += CLBYTES;
			ipc.ip_data -= CLBYTES;
			ipc.ip_addr2 += CLBYTES;
		}
		kmem_free(ipc.ip_bigbuf, CLBYTES);
		ipc.ip_bigbuf = NULL;
		break;

	case PTRACE_WRITEDATA:
	case PTRACE_WRITETEXT:
		ipc.ip_bigbuf = kmem_alloc(CLBYTES);
		while (ipc.ip_data > 0) {
			ipc.ip_req = uap->req;
			ipc.ip_nbytes = MIN(ipc.ip_data, CLBYTES);
			if (copyin(ipc.ip_addr2, ipc.ip_bigbuf, 
			    (u_int)ipc.ip_nbytes) != 0) {
				ipc.ip_error = 1;
				break;
			}
			runchild(p);
			ipc.ip_addr += CLBYTES;
			ipc.ip_data -= CLBYTES;
		}
		kmem_free(ipc.ip_bigbuf, CLBYTES);
		ipc.ip_bigbuf = NULL;
		break;

	default:
		runchild(p);
		u.u_r.r_val1 = ipc.ip_data;
		break;
	}

	if (ipc.ip_error != 0)
		u.u_error = EIO;

	ipc.ip_lock = 0;
	wakeup((caddr_t)&ipc);
}

/*
 * Set the child as runnable and go to sleep waiting for him
 * to do his part.
 */
runchild(p)
struct proc *p;
{

	while ((int)ipc.ip_req > (int)PTRACE_CHILDDONE) {
		if (p->p_stat == SSTOP)
			setrun(p);
		(void) sleep((caddr_t)&ipc, IPCPRI);
	}
}

/*
 * Code that the child process
 * executes to implement the command
 * of the parent process in tracing.
 */
procxmt()
{
	register enum ptracereq req;
	register struct text *xp;
	register int i, c;
	register int *p;
	register char *cp;
	register caddr_t a;
#ifdef sun3
	extern short fppstate;
#endif sun3

	if (ipc.ip_lock != u.u_procp->p_pid)
		return (0);
	u.u_procp->p_slptime = 0;
	req = ipc.ip_req;
	ipc.ip_req = PTRACE_CHILDDONE;
	switch (req) {

	/* read user I */
	case PTRACE_PEEKTEXT:
		if (!useracc(ipc.ip_addr, NBPW, B_READ))
			goto error;
		ipc.ip_data = fuiword(ipc.ip_addr);
		break;

	/* read user D */
	case PTRACE_PEEKDATA:
		if (!useracc(ipc.ip_addr, NBPW, B_READ))
			goto error;
		ipc.ip_data = fuword(ipc.ip_addr);
		break;

	/* read u */
	case PTRACE_PEEKUSER:
		i = (int)ipc.ip_addr;
		if (i < 0 || i >= ctob(UPAGES))
			goto error;
		ipc.ip_data = *(int *)PHYSOFF(&u, i);
		break;

	/* write user I */
	/* Must set up to allow writing */
	case PTRACE_POKETEXT:
		/*
		 * If text, must assure exclusive use
		 */
		if (xp = u.u_procp->p_textp) {
			struct vattr vattr;
			struct vnode *vp;

			vp = xp->x_vptr;
			VOP_GETATTR(vp, &vattr, u.u_cred);
			if (xp->x_count != 1 || (vattr.va_mode & VSVTX))
				goto error;
			vp->v_flag |= VTEXTMOD;
		}
		if ((i = suiword(ipc.ip_addr, ipc.ip_data)) < 0) {
			if (chgprot(ipc.ip_addr, (long)RW) &&
			    chgprot((ipc.ip_addr + NBPW - 1), (long)RW))
				i = suiword(ipc.ip_addr, ipc.ip_data);
			(void) chgprot(ipc.ip_addr, (long)RO);
			(void) chgprot((ipc.ip_addr + NBPW - 1), (long)RO);
		}
		if (i < 0)
			goto error;
		if (xp)
			xp->x_flag |= XWRIT;
		break;

	/* write user D */
	case PTRACE_POKEDATA:
		if (suword(ipc.ip_addr, 0) < 0)
			goto error;
		(void) suword(ipc.ip_addr, ipc.ip_data);
		break;

	/* write u */
	case PTRACE_POKEUSER:
		i = (int) ipc.ip_addr;
		p = (int *)PHYSOFF(&u, i);
		for (i = 0; i < NIPCREG; i++)
			if (p == &u.u_ar0[ipcreg[i]])
				goto ok;
		if (p == &u.u_ar0[PS]) {
			ipc.ip_data |= PSL_USERSET;
			ipc.ip_data &=  ~PSL_USERCLR;
			goto ok;
		}
		goto error;

	ok:
		*p = ipc.ip_data;
		break;

	/* set signal and continue */
	/* one version causes a trace-trap */
	case PTRACE_SINGLESTEP:
	/* one version stops tracing */
	case PTRACE_DETACH:
	case PTRACE_CONT:
		if ((int)ipc.ip_addr != 1)
			u.u_ar0[PC] = (int)ipc.ip_addr;
		if ((unsigned)ipc.ip_data > NSIG)
			goto error;
		u.u_procp->p_cursig = ipc.ip_data;	/* see issig */
		if (req == PTRACE_SINGLESTEP) 
			u.u_ar0[PS] |= PSL_T;
		else if (req == PTRACE_DETACH) {
			u.u_procp->p_flag &= ~STRC;
			u.u_procp->p_tptr = 0;
			if (xp = u.u_procp->p_textp) {
				/*
				 * Trust that the debugger has not
				 * left any changes in the text
				 * when using PTRACE_DETACH...
				 */
				xp->x_vptr->v_flag &= ~VTEXTMOD;
			}
		}
		wakeup((caddr_t)&ipc);
		return (1);

	/* force exit */
	case PTRACE_KILL:
		u.u_procp->p_flag &= ~STRC;
		u.u_procp->p_tptr = 0;
		wakeup((caddr_t)&ipc);
		exit(u.u_procp->p_cursig);

	/* read registers - copy from the u area to the ipc buffer */
	case PTRACE_GETREGS:
		ipc.ip_regs = *(struct regs *) u.u_ar0;
		break;

	/* write registers - copy from the ipc buffer to the u area */
	case PTRACE_SETREGS:
		*(struct regs *) u.u_ar0 = ipc.ip_regs;
		u.u_ar0[PS] |= PSL_USERSET;
		u.u_ar0[PS] &=  ~PSL_USERCLR;
		break;

	/* read data segment buffer */
	case PTRACE_READDATA:
		if (copyin(ipc.ip_addr, ipc.ip_bigbuf,
		    (u_int)ipc.ip_nbytes) != 0)
			goto error;
		break;

	/* write data segment buffer */
	case PTRACE_WRITEDATA:
		if (copyout(ipc.ip_bigbuf, ipc.ip_addr,
		    (u_int)ipc.ip_nbytes) != 0)
			goto error;
		break;

	/* read text segment buffer */
	case PTRACE_READTEXT:
		if (!useracc(ipc.ip_addr, (u_int)ipc.ip_nbytes, B_READ))
			goto error;

		for (c = 0, p = (int *)ipc.ip_bigbuf, a = ipc.ip_addr;
		    c < ipc.ip_nbytes - NBPW + 1; c += NBPW, a += NBPW, p++)
			*p = fuiword(a);

		for (cp = (char *)p; c < ipc.ip_nbytes; c++, a++, cp++)
			*cp = fuibyte(a);

		break;

	/* write text segment buffer */
	/* Must set up to allow writing - grossly inefficient */
	case PTRACE_WRITETEXT:
		/*
		 * If text, must assure exclusive use
		 */
		if (xp = u.u_procp->p_textp) {
			struct vattr vattr;
			struct vnode *vp;

			vp = xp->x_vptr;
			VOP_GETATTR(vp, &vattr, u.u_cred);
			if (xp->x_count != 1 || (vattr.va_mode & VSVTX))
				goto error;
			vp->v_flag |= VTEXTMOD;
		}

		for (c = 0, p = (int *)ipc.ip_bigbuf, a = ipc.ip_addr;
		    c < ipc.ip_nbytes - NBPW + 1; c += NBPW, a += NBPW, p++) {
			if ((i = suiword(a, *p)) < 0) {
				if (chgprot(a, (long)RW) &&
				    chgprot((a + NBPW - 1), (long)RW))
					i = suiword(a, *p);
				(void) chgprot(a, (long)RO);
				(void) chgprot((a + NBPW - 1), (long)RO);
			}
			if (i < 0)
				goto error;
			if (xp)
				xp->x_flag |= XWRIT;
		}

		for (cp = (char *)p; c < ipc.ip_nbytes; c++, a++, cp++) {
			if ((i = suibyte(a, *p)) < 0) {
				if (chgprot(a, (long)RW))
					i = suibyte(a, *p);
				(void) chgprot(a, (long)RO);
			}
			if (i < 0)
				goto error;
			if (xp)
				xp->x_flag |= XWRIT;
		}
		break;

#ifdef sun3
	/*
	 * Read floating point registers -
	 * Copy from the u area to the ipc buffer,
	 * after setting the fps_flags and fps_code
	 * from the internal fpp info.
	 */
	case PTRACE_GETFPREGS:
		if (fppstate == 0) {
			ipc.ip_error = 1;
		} else {
			u.u_fp_status.fps_flags = EXT_FPS_FLAGS(&u.u_fp_istate);
			u.u_fp_status.fps_code = u.u_code;
			ipc.ip_fp_status = u.u_fp_status;
		}
		break;

	/*
	 * Write floating point registers -
	 * Copy from the ipc buffer to the u area,
	 * and set u.u_code from the code in fp_status,
	 * then initialize the fpp registers.
	 */
	case PTRACE_SETFPREGS:
		if (fppstate == 0) {
			ipc.ip_error = 1;
		} else {
			u.u_fp_status = ipc.ip_fp_status;
			u.u_code = u.u_fp_status.fps_code;
			setfppregs();
		}
		break;
#if NFPA > 0
	/* Read FPA registers from u area and FPA board to ipc buffer. */
	case PTRACE_GETFPAREGS:
		if ((!u.u_fpa_flags) || (fpa->fp_pipe_status & FPA_WAIT2))
			goto error;
		ipc.ip_fpa_flags = u.u_fpa_flags;
		fpa_save(); /* force a save to u area */
		ipc.ip_fpa_status = u.u_fpa_status;
		fpa->fp_clear_pipe = 0; /* clear FPA pipe to read data regs */
		bcopy((char *)fpa->fp_data, (char *)ipc.ip_fpa_data,
		      sizeof (fpa->fp_data)); /* copy from fpa */
		fpa_dorestore(); /* restore u area to FPA */
		break;
	/*
	 * Write FPA registers from ipc buffer to the U area and FPA
	 * board.  U.u_fpa_flags, FPA STATE register, and WSTATUS register
	 * are protected from being written.
	 * Fpa_restore() is needed, o/w u.u_fpa_status is erased by
	 * fpa context save during the coming context switch.
	 */
	 case PTRACE_SETFPAREGS:
		if ((!u.u_fpa_flags) || (fpa->fp_pipe_status & FPA_WAIT2))
			goto error;
		/* STATE reg is protected from being modified */
		ipc.ip_fpa_state = u.u_fpa_status.fpas_state;
		u.u_fpa_status = ipc.ip_fpa_status;
		fpa->fp_clear_pipe = 0; /* clear FPA pipe to read data regs */
		bcopy((char *)ipc.ip_fpa_data, (char *)fpa->fp_data,
		      sizeof (fpa->fp_data)); /* copy to fpa */
		fpa_dorestore();
		break;
#endif NFPA > 0
#endif sun3

	default:
	error:
		ipc.ip_error = 1;
	}
	wakeup((caddr_t)&ipc);
	return (0);
}
