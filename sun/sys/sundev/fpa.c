#ifndef lint
static  char sccsid[] = "@(#)fpa.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Sun Floating Point Accelerator driver
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"
#include "../h/ioctl.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/systm.h"
#include "../sundev/mbvar.h"
#include "../sundev/fpareg.h"
#include "../sun3/enable.h"

int	fpaprobe();

struct mb_device *fpainfo[1];		/* XXX only support one board */

struct mb_driver fpadriver = {
	fpaprobe, 0, 0, 0, 0, 0,
	sizeof (struct fpa_device), "fpa", fpainfo, 0, 0, 0
};

/* fpa_procp[i] points to the proc that is using FPA context i. */
struct proc *fpa_procp[FPA_NCONTEXTS];
/*
 * Fpa_exist == 0 if FPA does not exist.  Fpa_exist == 1 if there is a
 * functioning FPA attached.  Fpa_exist == -1 if the FPA is disabled
 * due to FPA hardware problems.
 */
short	fpa_exist;
enum fpa_state	fpa_state = FPA_SINGLE_USER;
short	fpa_last_ctx = 0; /* last context number allocated */

extern  struct fileops vnodefops;
extern	u_char enablereg;	/* soft copy of ENABLE register */
extern  short fppstate;		/* whether 68881 exists */
extern	on_enablereg();		/* turn on a bit in enable reg */
extern	off_enablereg();	/* turn off a bit in enable reg */

/*
 * fpaprobe() checks if an FPA is attached.  It assigns the address of
 * FPA to global variable fpa and turns on fpa_exist if the FPA is there.
 */
/*ARGSUSED*/
fpaprobe(reg, unit)
	caddr_t reg;
	int unit;
{
	long i;

	fpa = (struct fpa_device *)reg;
	on_enablereg((u_char)ENA_FPA); /* turn on before access FPA */
	if (peekl((long *)&fpa->fp_ierr, &i) == 0)
		fpa_exist = 1;
	else 
		fpa_exist = 0;
	off_enablereg((u_char)ENA_FPA); /* It'll be truned on by fpaopen */
	return (fpa_exist);
}

/*
 * fpaopen() is called before the first FPA instruction is executed.
 * It makes sure there is an FPA board available, allocates and initializes
 * an FPA context, marks this process is using the FPA,
 * and turns on the FPA enable bit in the enable register.
 * Minor device number is changed to associate a unique device number
 * with an FPA context such that fpaclose() is called when we close
 * an FPA context.  THE MINOR DEVICE NUMBER OF /dev/fpa IN /etc/MAKEDEV
 * SHOULD NOT BE SET TO 0~31.
 */
/*ARGSUSED*/
fpaopen(dev, flag, truedev)
	dev_t dev;
	int flag;
	dev_t *truedev;
{
	register int context_no;
	register struct fpa_device *rfpa = fpa;

	if (!fpa_exist)
		return (ENXIO);		/* FPA board not exist */
	if (!fppstate)
		return (ENOENT);	/* 68881 chip not exist */
	if (fpa_state == FPA_DISABLED)
		return (ENETDOWN);	/* FPA is disabled */
	/* if FPA is being used and not in the middle of a fork, error */
	if (u.u_fpa_flags && !(u.u_fpa_flags & U_FPA_INFORK))
		return (EEXIST);	/* multiple open */

	/* Only ONE root can use fpa while in FPA_SINGLE_USER */
	if (fpa_state == FPA_SINGLE_USER)
		if (!suser() || fpa_busy())
			return (EIO);
	if ((context_no = fpa_alloc_context()) < 0)
		return (EBUSY);

	*truedev = makedev(major(dev), context_no);

	/*
	 * Skip updates to u and FPA registers if we are doing fork.
	 * FPA registers of the child context will be updated from
	 * child's u at context switch time when the child starts to run.
	 */
	if (u.u_fpa_flags & U_FPA_INFORK)
		return (0);

	/* Enable FPA and update u.u_fpa_*. */
	on_enablereg((u_char)ENA_FPA);
	u.u_fpa_flags = U_FPA_USED;
	u.u_fpa_fmtptr = NULL;
	u.u_fpa_pc = 0;
	u.u_fpa_status.fpas_state = context_no;
	/* init FPA regs */
	rfpa->fp_clear_pipe = 0; /* clear FPA pipe before write to STATE reg */
	rfpa->fp_state = u.u_fpa_status.fpas_state;
	/* Do a destructive RAM test if in FPA_MULTI_USER state */
	if (fpa_state == FPA_MULTI_USER) {
		rfpa->fp_restore_mode3_0 = FPA_MODE3_INITVAL;
		if (fparamtest() < 0) {
			printf("FPA RAM test fails, FPA is shutdown!\n");
			fpa_shutdown(); /* if RAM test fails, shut down FPA */
			return (ENETDOWN);
		}
	}
	rfpa->fp_imask = rfpa->fp_ierr = rfpa->fp_load_ptr = 0;
	return (0);
}

/*
 * fpaioctl() is called to turn on/off the load enable bit and the
 * regram access bit of the FPA STATE register.  Download program
 * calls ioctl() to change state from FPA_SINGLE_USER to FPA_MULTI_USER
 * after the microcode has been downloaded successfully.
 * Also, diagnostic programs disables FPA through ioctl().
 * FPA_GET_DATAREGS is only used by gcore() to read FPA data registers.
 */
/*ARGSUSED*/
fpaioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd, flag;
	caddr_t data;
{

	if (!suser() && cmd != FPA_GET_DATAREGS)
		return (EPERM);
	if ((fpa->fp_pipe_status & FPA_PIPE_MASK) != FPA_PIPE_CLEAR)
		return (EPIPE);

	switch (cmd) {
	case FPA_ACCESS_ON:
		fpa->fp_state |= FPA_ACCESS_BIT;
		break;
	case FPA_ACCESS_OFF:
		fpa->fp_state &= ~FPA_ACCESS_BIT;
		break;
	case FPA_LOAD_ON:
		fpa->fp_state |= FPA_LOAD_BIT;
		break;
	case FPA_LOAD_OFF:
		fpa->fp_state &= ~FPA_LOAD_BIT;
		break;
	case FPA_GET_DATAREGS: {
			union fpa_qtr_dregs *p = (union fpa_qtr_dregs *)data;
			int count = p->fq_params.fqp_count,
			    state_reg = p->fq_params.fqp_state;

			fpa_get_dataregs(state_reg, count, data);
		}
		break;
	case FPA_FAIL:
		printf(data);
		fpa_shutdown();
		break;
	case FPA_INIT_DONE:
		fpa_state = FPA_MULTI_USER;
		break;
	default:
		return (EINVAL);
	}
	return (0);
}

/*
 * fpaclose() is called when exit() or execve() is called.
 * It marks that this process is not using FPA, runs a destructive
 * RAM test, frees the FPA context, and turns off the FPA enable bit
 * in the ENABLE register.
 */
/*ARGSUSED*/
fpaclose(dev, flag)
	dev_t dev;
	int flag;
{
	short fpa_stk_allocsize;

	if (u.u_fpa_fmtptr != NULL) {
		fpa_stk_allocsize = *(short *)(u.u_fpa_fmtptr + sizeof (short));
		if (fpa_stk_allocsize <= 0)
			panic("fpaclose: fmt inconsistent!");
		kmem_free(u.u_fpa_fmtptr, (u_int)fpa_stk_allocsize);
		u.u_fpa_fmtptr = NULL;
	}
	/* Skip any microcode execution if microcode is not loaded. */
	if (fpa_state == FPA_MULTI_USER) {
		fpa->fp_clear_pipe = 0; /* clear FPA pipe before the RAM test */
		fpa->fp_restore_mode3_0 = FPA_MODE3_INITVAL;
		if (fparamtest() < 0) {
			printf("FPA RAM test fails, FPA is shutdown!\n");
			uprintf("FPA test fails, computation result may be wrong!\n");
			fpa_shutdown();
		}
	}
	u.u_fpa_flags = 0;		/* mark that FPA is not used */
	fpa_free_context(u.u_fpa_status.fpas_state & FPA_CONTEXT);
	/* turn off bit EN.FPA on both Unix and hard enable register */
	off_enablereg((u_char)ENA_FPA);
}


/*
 * fpa_alloc_context() allocates an FPA context.
 * It returns the context number of the new context or -1.
 * If fpa_procp[i] == NULL, FPA context i is free.
 * To allow diagnostic programs check the RAM of every context,
 * we don't always start allocating cantext from context 0.
 */
int
fpa_alloc_context()
{
	register int i;
	register int j = (int)fpa_last_ctx;

	for (i = 0; i < FPA_NCONTEXTS; i++) {
		j = (j + 1) % FPA_NCONTEXTS;
		if (fpa_procp[j] == NULL) {
			fpa_procp[j] = u.u_procp;
			fpa_last_ctx = j;
			return (j);
		}
	}
	return (-1);
}

/*
 * fpa_free_context(context_no) frees this FPA context.
 */
fpa_free_context(context_no)
	u_int context_no;
{

	if (fpa_procp[context_no] == NULL)
		panic("fpa_free_context");
	fpa_procp[context_no] = NULL;
}

/*
 * fpa_busy() returns 1 if FPA is being used, 0 if nobody is using FPA.
 */
int
fpa_busy()
{
	register i;

	for (i = 0; i < FPA_NCONTEXTS; i++)
		if (fpa_procp[i])
			return (1);
	return (0);
}

/*
 * fparamtest() starts a destructive RAM test, waits until the
 * the pipe is ready, checks if the test is succeeded, and returns
 * 0 if the test succeeds or -1 if the test fails.
 */
int
fparamtest()
{
	register struct fpa_device *rfpa = fpa;

	rfpa->fp_destructive_test = 0;
	CDELAY(rfpa->fp_pipe_status & FPA_STABLE, 300);
	if ( (rfpa->fp_pipe_status & FPA_STABLE) &&
	    !(rfpa->fp_pipe_status & FPA_HUNG))
		return (0);
	else {
		if (!(rfpa->fp_pipe_status & FPA_STABLE))
			printf("fparamtest: nonstable after delay 300 us.\n");
		else
			printf("fparamtest: pipe is hung\n");
		return (-1);
	}
}

/*
 * fpa_shutdown() is called to disable FPA and kill all processes
 * using FPA.  It is called when an FPA hardware problem is found.
 * It marks FPA is unavailable and kills all processes using FPA.
 */
fpa_shutdown()
{
	register int i;

	fpa_state = FPA_DISABLED;
	for (i = 0; i < FPA_NCONTEXTS; i++)
		if (fpa_procp[i])
			psignal(fpa_procp[i], SIGKILL);
}

/*
 * fpa_get_dataregs() is called from fpaioctl() to return a quarter
 * of FPA data registers to the user.  State_reg is that of the probed proc,
 * count tells which quarter the user requests, data is a pointer to
 * where we will dump the value of data registers.
 */
fpa_get_dataregs(state_reg, count, data)
	int state_reg, count;
	caddr_t data;
{
	register int i = 0;
	register struct fpa_device *rfpa = fpa;
	int saved_state = rfpa->fp_state;

	/* wait till FPA pipe is stable */
	CDELAY(rfpa->fp_pipe_status & FPA_STABLE, 300);
	if (!(rfpa->fp_pipe_status & FPA_STABLE)) {
		printf("FPA pipe is not stable\n");
		uprintf("FPA pipe not stable, cannot get FPA data registers\n");
		fpa_shutdown();
		return;
	}
	rfpa->fp_state = state_reg; /* change state register */
	/* copy data registers */
	for (i = 0; i < FPA_QTR_SIZE; i++)
		*((fpa_long *)data + i) =
		    rfpa->fp_data[count * FPA_QTR_SIZE + i];
	rfpa->fp_state = saved_state; /* restore state register */
}

/*
 * fpa_save() is called to force a context save before clearing 
 * fpa pipe.  It is followed by calling fpa_dorestore().
 * It is NOT called during context switch.
 */
fpa_save()
{
	CDELAY(fpa->fp_pipe_status & FPA_STABLE, 300);
	if (fpa->fp_pipe_status & FPA_STABLE)
		u.u_fpa_status = *((struct fpa_status *)&fpa->fp_state);
	else {
		printf("FPA pipe is not stable\n");
		fpa_shutdown();
	}
}

#define u_fpas_state		u_fpa_status.fpas_state
#define u_fpas_imask		u_fpa_status.fpas_imask
#define u_fpas_load_ptr		u_fpa_status.fpas_load_ptr
#define u_fpas_ierr		u_fpa_status.fpas_ierr
#define u_fpas_mode3_0		u_fpa_status.fpas_mode3_0
#define u_fpas_wstatus		u_fpa_status.fpas_wstatus
#define u_fpas_act_instr	u_fpa_status.fpas_act_instr
#define u_fpas_nxt_instr	u_fpa_status.fpas_nxt_instr
#define u_fpas_act_d1half	u_fpa_status.fpas_act_d1half
#define u_fpas_act_d2half	u_fpa_status.fpas_act_d2half
#define u_fpas_nxt_d1half	u_fpa_status.fpas_nxt_d1half
#define u_fpas_nxt_d2half	u_fpa_status.fpas_nxt_d2half
/*
 * fpa_restore() is called from vax.s to turn on Enable Register and
 * check if we are the last process using FPA.  We skip restoring FPA
 * context if we are the last process using FPA.
 */
fpa_restore()
{

	on_enablereg((u_char)ENA_FPA);
	if ((fpa->fp_state & FPA_STATE_BITS) !=
	    (u.u_fpas_state & FPA_STATE_BITS))
		fpa_dorestore();
}

/*
 * fpa_dorestore() is called from fpa_restore if we are not the last
 * process using FPA and from procxmt() to restore FPA context
 * after calling fpa_save() and clearing FPA pipe.
 */
fpa_dorestore()
{
	register u_int	instr_addr;
	register struct fpa_device *rfpa = fpa;

	rfpa->fp_clear_pipe = 0; /* clear FPA pipe */
	rfpa->fp_state = u.u_fpas_state;
	rfpa->fp_imask = u.u_fpas_imask;
	rfpa->fp_load_ptr = u.u_fpas_load_ptr;
	rfpa->fp_ierr = u.u_fpas_ierr;

	/*
	 * If microcode is not loaded, don't touch MODE3_0, WSTATUS, and
	 * shadow registers.
	 */
	if (!(rfpa->fp_state & FPA_LOAD_BIT)) {
		/* restore MODE3_0 */
		rfpa->fp_restore_mode3_0 = u.u_fpas_mode3_0 & FPA_MODE_BITS;
		/* restore WSTATUS */
		if (u.u_fpas_wstatus & FPA_STATUS_VALID)
			/* restore WSTATUS */
			rfpa->fp_restore_wstatus = u.u_fpas_wstatus;
		/* update shadow registers */
		rfpa->fp_restore_shadow = 0;
	}

	/*
	 * Restore FPA pipe.
	 * If (v bit is valid)
	 *	write corresponding data to the address specified by the
	 *	instruction half; (v is 0 => valid!)
	 */
	if (!(u.u_fpas_act_instr & FPA_FIRST_V)) {
		instr_addr = (u.u_fpas_act_instr & FPA_FIRST_HALF)
			>> FPA_ADDR_SHIFT;
		*( (u_int *)((u_int)rfpa + instr_addr) ) = u.u_fpas_act_d1half;
	}
	if (!(u.u_fpas_act_instr & FPA_SECOND_V)) {
		instr_addr = u.u_fpas_act_instr & FPA_SECOND_HALF;
		*( (u_int *)((u_int)rfpa + instr_addr) ) = u.u_fpas_act_d2half;
	}
	if (!(u.u_fpas_nxt_instr & FPA_FIRST_V)) {
		instr_addr = (u.u_fpas_nxt_instr & FPA_FIRST_HALF)
			>> FPA_ADDR_SHIFT;
		*( (u_int *)((u_int)rfpa + instr_addr) ) = u.u_fpas_nxt_d1half;
	}
	if (!(u.u_fpas_nxt_instr & FPA_SECOND_V)) {
		instr_addr = (u.u_fpas_nxt_instr & FPA_SECOND_HALF);
		*( (u_int *)((u_int)rfpa + instr_addr) ) = u.u_fpas_nxt_d2half;
	}
}

/* 
 * fpa_copydata(new_context) copies data regs from the FPA context of
 * current process to FPA context new_context.
 */
fpa_copydata(new_context)
	int new_context;
{
	register int	old_state, new_state, /* value of STATE reg */
			data_buffer1, /* to hold the most significant half */
			data_buffer2, /* to hold the least significant half */
			i;	/* count of the loop */
	register struct fpa_device *rfpa = fpa;
	
	old_state = rfpa->fp_state;
	new_state = (old_state & FPA_PBITS) | new_context;
	fpa_save(); /* before doing a clearpipe */
	rfpa->fp_clear_pipe = 0; /* clear FPA pipe to read data regs */

	for (i = 0; i < FPA_NDATA_REGS; i++) {
		rfpa->fp_state = old_state; /* switch to old context */
		data_buffer1 = rfpa->fp_data[i].fpl_data[0];
		data_buffer2 = rfpa->fp_data[i].fpl_data[1];
		rfpa->fp_state = new_state; /* switch to new context */
		rfpa->fp_data[i].fpl_data[0] = data_buffer1;
		rfpa->fp_data[i].fpl_data[1] = data_buffer2;
	}
	rfpa->fp_state = old_state; /* switch to original FPA context */
	fpa_restore(); /* restore to the state before coping data regs */
}

/*
 * fpa_fork_context(childp, pnfp, pnew_context) calls falloc()
 * to get a struct file, inits it as in copen(),
 * calls VOP_OPEN() to get a vnode, assigns new file pointer
 * to *pnfp and new FPA context number to *pnew_context,
 * and copies the FPA data registers of parent to the new FPA context.
 * fpa_fork_context() has the side effect of setting fd to u.u_fpa_flags,
 * which is used in the child process in fork1().
 */
fpa_fork_context(childp, pnfp, pnew_context)
	struct proc *childp;
	struct file **pnfp;
	register int *pnew_context;
{
	register struct file *fp, *ofp;
	struct file *tmp_fp;
	register int i;
	register struct vnode *vp; /* to /dev/fpa Snode of parent */
	struct vnode *nvp;	   /* to that of child */

	/* Set the fd of /dev/fpa to u.u_fpa_flags. */
	for (i = 0; i < NOFILE; i++)
		if ((ofp = u.u_ofile[i]) != NULL) {
			vp = (struct vnode *)(ofp->f_data);
			if ((vp->v_type == VCHR) &&
			    (major(vp->v_rdev) < nchrdev))
				if (cdevsw[major(vp->v_rdev)].d_open == fpaopen)
					goto found;
		}

	panic("fpafork: cannot find fpa file");
found:
	u.u_fpa_flags |= i & U_FPA_FDBITS; /* this fd is used in fork1() */
	u.u_fpa_flags |= U_FPA_INFORK; /* to avoid dup fpa open */
	nvp = specvp(vp, makedev(major(vp->v_rdev), FPA_NCONTEXTS));
	if (VOP_OPEN(&nvp, 0, u.u_cred) && nvp) {
		/* VOP_OPEN() returns non-zero in case of errors */
		vn_rele(nvp);
		u.u_fpa_flags &= ~U_FPA_INFORK;
		return (-1);
	}
	u.u_fpa_flags &= ~U_FPA_INFORK;
	*pnew_context = minor(nvp->v_rdev);
	fpa_procp[*pnew_context] = childp; /* Put child procp to fpa_procp. */

	/*
	 * Find a (struct file) for the file /dev/fpa of child.
	 * Falloc() needs at least one empty slot in u.u_ofile[],
	 * but the child will use the same slot holding file /dev/fpa
	 * as the parent.  So, we make an available entry in
	 * u.u_ofile[] for falloc() and restore it after falloc() returns.
	 */
	tmp_fp = u.u_ofile[NOFILE - 1];
	u.u_ofile[NOFILE -1] = NULL;
	if ((fp = falloc()) == NULL) {
		/* There is no free struct file */
		fpa_free_context((u_int)*pnew_context);
		u.u_ofile[NOFILE -1] = tmp_fp;
		vn_rele(nvp);
		return (-1);
	}
	for (i = 0; u.u_ofile[i] != fp && i < NOFILE; i++)
		;
	if (u.u_ofile[i] == fp)
		u.u_ofile[i] = NULL; /* We are still in parent u */
	else
		panic("fpa_falloc");
	u.u_ofile[NOFILE -1] = tmp_fp; /* restore u.u_ofile[NOFILE -1] */
	*pnfp = fp; /* Pass back the new (struct file *)fp for the child */

	/* init struct file as part of copen() */
	fp->f_flag = ofp->f_flag & FMASK; /* filemode */
	fp->f_type = DTYPE_VNODE;
	fp->f_data = (caddr_t)nvp;
	fp->f_ops = &vnodefops;

	fpa_copydata(*pnew_context); /* copy FPA data regs to child context */
	return (0);
}
