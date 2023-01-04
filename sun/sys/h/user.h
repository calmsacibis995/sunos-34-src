/*	@(#)user.h 1.1 86/09/25 SMI; from UCB 6.2 83/11/21	*/

#ifndef __USER_HEADER__
#define __USER_HEADER__

#ifdef KERNEL
#include "../machine/pcb.h"
#ifdef sun
#include "../machine/reg.h"
#endif
#include "../h/dmap.h"
#include "../h/time.h"
#include "../h/resource.h"
#include "../h/exec.h"
#else KERNEL
#include <machine/pcb.h>
#ifdef sun
#include <machine/reg.h>
#endif
#include <sys/dmap.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/exec.h>
#endif KERNEL

/*
 * Per process structure containing data that
 * isn't needed in core when the process is swapped out.
 * A redzone is set below the user structure as
 * a safe guard against kernel stack overflow.
 */
 
#define	SHSIZE		sizeof (struct exec)
#define	MAXCOMLEN	16		/* <= MAXNAMLEN, >= sizeof (a_comm) */
 
struct	user {
	char	u_stack[KERNSTACK];	/* the kernel stack */
	struct	pcb u_pcb;
	struct	proc *u_procp;		/* pointer to proc structure */
	int	*u_ar0;			/* address of users saved R0 */
	char	u_comm[MAXCOMLEN + 1];

/* syscall parameters, results and catches */
	int	u_arg[8];		/* arguments to current system call */
	int	*u_ap;			/* pointer to arglist */
	label_t	u_qsave;		/* for non-local gotos on interrupts */
	char	u_error;		/* return error code */
	union {				/* syscall return values */
		struct	{
			int	R_val1;
			int	R_val2;
		} u_rv;
#define	r_val1	u_rv.R_val1
#define	r_val2	u_rv.R_val2
		off_t	r_off;
		time_t	r_time;
	} u_r;
#define	u_rval1 u_r.r_val1
#define	u_rval2 u_r.r_val2
	char	u_eosys;		/* special action on end of syscall */

/* 1.1 - processes and protection */
	struct ucred *u_cred;		/* user credentials (uid, gid, etc) */
#define	u_uid	u_cred->cr_uid
#define	u_gid	u_cred->cr_gid
#define	u_groups u_cred->cr_groups
#define	u_ruid	u_cred->cr_ruid
#define	u_rgid	u_cred->cr_rgid

/* 1.2 - memory management */
	size_t	u_tsize;		/* text size (clicks) */
	size_t	u_dsize;		/* data size (clicks) */
	size_t	u_ssize;		/* stack size (clicks) */
	struct	dmap u_dmap;		/* disk map for data segment */
	struct	dmap u_smap;		/* disk map for stack segment */
	struct	dmap *u_cdmap;		/* ptr to temp data segment disk map */
	struct	dmap *u_csmap;		/* ptr to temp stack segment disk map */
	label_t u_ssave;		/* label variable for swapping */
	size_t	u_odsize, u_ossize;	/* for (clumsy) expansion swaps */
	time_t	u_outime;		/* user time at last sample */

/* 1.3 - signal management */
	int	(*u_signal[NSIG])();	/* disposition of signals */
	int	u_sigmask[NSIG];	/* signals to be blocked */
	int	u_sigonstack;		/* signals to take on sigstack */
	int	u_oldmask;		/* saved mask from before sigpause */
	int	u_code;			/* ``code'' to trap */
	struct	sigstack u_sigstack;	/* sp & on stack state variable */
#define	u_onstack	u_sigstack.ss_onstack
#define	u_sigsp		u_sigstack.ss_sp

/* 1.4 - descriptor management */
	struct	file *u_ofile[NOFILE];	/* file structures for open files */
	char	u_pofile[NOFILE];	/* per-process flags of open files */
#define	UF_EXCLOSE 	0x1		/* auto-close on exec */
#define	UF_MAPPED 	0x2		/* mapped from device */
#define UF_FDLOCK	0x4		/* file desc locked (SysV style) */
	struct	vnode *u_cdir;		/* current directory */
	struct	vnode *u_rdir;		/* root directory of current process */
	struct	tty *u_ttyp;		/* controlling tty pointer */
	dev_t	u_ttyd;			/* controlling tty dev */
	short	u_cmask;		/* mask for file creation */

/* 1.5 - timing and statistics */
	struct	rusage u_ru;		/* stats for this proc */
	struct	rusage u_cru;		/* sum of stats for reaped children */
	struct	itimerval u_timer[3];
	int	u_XXX[3];
	time_t	u_start;
	short	u_acflag;

/* 1.6 - resource controls */
	struct	rlimit u_rlimit[RLIM_NLIMITS];

/* BEGIN TRASH */
	union {
		struct exec Ux_A;	/* header of executable file */
		char ux_shell[SHSIZE];	/* #! and name of interpreter */
	} u_exdata;
#define	ux_mach		Ux_A.a_machtype
#define	ux_mag		Ux_A.a_magic
#define	ux_tsize	Ux_A.a_text
#define	ux_dsize	Ux_A.a_data
#define	ux_bsize	Ux_A.a_bss
#define	ux_ssize	Ux_A.a_syms
#define	ux_entloc	Ux_A.a_entry
#define	ux_unused	Ux_A.a_trsize
#define	ux_relflg	Ux_A.a_drsize
#ifdef sun
	int	u_lofault;		/* catch faults in locore.s */
	struct	hole {			/* a data space hole (no swap space) */
		int	uh_first;	/* first data page in hole */
		int	uh_last;	/* last data page in hole */
	} u_hole;
	/* sun2 only */
	int	u_memropc[12];		/* state of ropc */
	struct skyctx {
		u_int	 usc_regs[8];	/* the Sky registers */
		short	 usc_cmd;	/* current command */
		short	 usc_used;	/* user is using Sky */
	} u_skyctx;
	/* end sun2 only */
	/* 68020/68881 only */
	struct fp_status u_fp_status;	/* user visible fpp state */
	struct fp_istate u_fp_istate;	/* internal fpp state */
	/* end 68020/68881 only */
	/* 68020/fpa only */
	short	u_fpa_flags; /* if zero, u_fpa_*'s are meaningless */
	struct fpa_status u_fpa_status; /* saved/restored on ctx switches */
	struct fpa_exception {
		caddr_t	fe_fmtptr; /* points to space holding bus error frame */
		int	fe_pc; /* pc of user code where the bus error occurs */
	} u_fpa_exception;
#define u_fpa_fmtptr		u_fpa_exception.fe_fmtptr
#define u_fpa_pc		u_fpa_exception.fe_pc
	/* end 68020/fpa only */
#endif sun
/* END TRASH */
	struct uprof {			/* profile arguments */
		short	*pr_base;	/* buffer base */
		u_int	 pr_size;	/* buffer size */
		u_int	 pr_off;	/* pc offset */
		u_int	 pr_scale;	/* pc scaling */
	} u_prof;
	int	u_sigintr;		/* signals that interrupt syscalls */
	int	u_sigreset;		/* signals that reset the handler when taken */
};

struct ucred {
	u_short	cr_ref;			/* reference count */
	short   cr_uid;			/* effective user id */
	short   cr_gid;			/* effective group id */
	int     cr_groups[NGROUPS];	/* groups, 0 terminated */
	short   cr_ruid;		/* real user id */
	short   cr_rgid;		/* real group id */
};

#ifdef KERNEL
#define	crhold(cr)	(cr)->cr_ref++
struct ucred *crget();
struct ucred *crcopy();
struct ucred *crdup();
#endif KERNEL

/* u_eosys values */
#define	JUSTRETURN	0
#define	RESTARTSYS	1
#define	SIMULATERTI	2
#define	REALLYRETURN	3

/* u_error codes */
#ifdef KERNEL
#include "../h/errno.h"
#else KERNEL
#include <errno.h>
#endif KERNEL

#ifdef KERNEL
#ifdef sun
#define	u	(*(struct user *)UADDR)
#else sun
extern	struct user u;
#endif sun
extern	struct user swaputl;
extern	struct user forkutl;
extern	struct user xswaputl;
extern	struct user xswap2utl;
extern	struct user pushutl;
extern	struct user vfutl;
#endif KERNEL

#ifdef sun
#define U_FPA_USED      0x0100
#define U_FPA_INFORK	0x80
#define U_FPA_FDBITS    0x7F
#endif

#endif !__USER_HEADER__
