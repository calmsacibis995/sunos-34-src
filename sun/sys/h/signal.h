/*	@(#)signal.h 1.1 86/09/25 SMI; from UCB 6.7 85/06/08	*/

/*
 * Copyright (c) 1982 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef	NSIG
#define NSIG	32

#define	SIGHUP	1	/* hangup */
#define	SIGINT	2	/* interrupt */
#define	SIGQUIT	3	/* quit */
#define	SIGILL	4	/* illegal instruction (not reset when caught) */
#ifdef vax
#define     ILL_RESAD_FAULT	0x0	/* reserved addressing fault */
#define     ILL_PRIVIN_FAULT	0x1	/* privileged instruction fault */
#define     ILL_RESOP_FAULT	0x2	/* reserved operand fault */
/* CHME, CHMS, CHMU are not yet given back to users reasonably */
#endif vax
#ifdef mc68000
#define     ILL_ILLINSTR_FAULT	0x10	/* illegal instruction fault */
#define     ILL_PRIVVIO_FAULT	0x20	/* privilege violation fault */
#define     ILL_COPROCERR_FAULT	0x34	/* [coprocesser protocol error fault] */
#define     ILL_TRAP1_FAULT	0x84	/* trap #1 fault */
#define     ILL_TRAP2_FAULT	0x88	/* trap #2 fault */
#define     ILL_TRAP3_FAULT	0x8c	/* trap #3 fault */
#define     ILL_TRAP4_FAULT	0x90	/* trap #4 fault */
#define     ILL_TRAP5_FAULT	0x94	/* trap #5 fault */
#define     ILL_TRAP6_FAULT	0x98	/* trap #6 fault */
#define     ILL_TRAP7_FAULT	0x9c	/* trap #7 fault */
#define     ILL_TRAP8_FAULT	0xa0	/* trap #8 fault */
#define     ILL_TRAP9_FAULT	0xa4	/* trap #9 fault */
#define     ILL_TRAP10_FAULT	0xa8	/* trap #10 fault */
#define     ILL_TRAP11_FAULT	0xac	/* trap #11 fault */
#define     ILL_TRAP12_FAULT	0xb0	/* trap #12 fault */
#define     ILL_TRAP13_FAULT	0xb4	/* trap #13 fault */
#define     ILL_TRAP14_FAULT	0xb8	/* trap #14 fault */
#endif mc68000
#define	SIGTRAP	5	/* trace trap (not reset when caught) */
#define	SIGIOT	6	/* IOT instruction */
#define	SIGEMT	7	/* EMT instruction */
#ifdef mc68000
#define     EMT_EMU1010		0x28	/* line 1010 emulator trap */
#define     EMT_EMU1111		0x2c	/* line 1111 emulator trap */
#endif mc68000
#define	SIGFPE	8	/* floating point exception */
#ifdef vax
#define     FPE_INTOVF_TRAP	0x1	/* integer overflow */
#define     FPE_INTDIV_TRAP	0x2	/* integer divide by zero */
#define     FPE_FLTOVF_TRAP	0x3	/* floating overflow */
#define     FPE_FLTDIV_TRAP	0x4	/* floating/decimal divide by zero */
#define     FPE_FLTUND_TRAP	0x5	/* floating underflow */
#define     FPE_DECOVF_TRAP	0x6	/* decimal overflow */
#define     FPE_SUBRNG_TRAP	0x7	/* subscript out of range */
#define     FPE_FLTOVF_FAULT	0x8	/* floating overflow fault */
#define     FPE_FLTDIV_FAULT	0x9	/* divide by zero floating fault */
#define     FPE_FLTUND_FAULT	0xa	/* floating underflow fault */
#endif vax
#ifdef mc68000
#define     FPE_INTDIV_TRAP	0x14	/* integer divide by zero */
#define     FPE_CHKINST_TRAP	0x18	/* CHK [CHK2] instruction */
#define     FPE_TRAPV_TRAP	0x1c	/* TRAPV [cpTRAPcc TRAPcc] instr */
#define     FPE_FLTBSUN_TRAP	0xc0	/* [branch or set on unordered cond] */
#define     FPE_FLTINEX_TRAP	0xc4	/* [floating inexact result] */
#define     FPE_FLTDIV_TRAP	0xc8	/* [floating divide by zero] */
#define     FPE_FLTUND_TRAP	0xcc	/* [floating underflow] */
#define     FPE_FLTOPERR_TRAP	0xd0	/* [floating operand error] */
#define     FPE_FLTOVF_TRAP	0xd4	/* [floating overflow] */
#define     FPE_FLTNAN_TRAP	0xd8	/* [floating Not-A-Number] */
#ifdef sun
#define     FPE_FPA_ENABLE	0x400	/* [FPA not enabled] */
#define     FPE_FPA_ERROR	0x404	/* [FPA arithmetic exception] */
#endif sun
#endif mc68000
#define	SIGKILL	9	/* kill (cannot be caught or ignored) */
#define	SIGBUS	10	/* bus error */
#define	SIGSEGV	11	/* segmentation violation */
#define	SIGSYS	12	/* bad argument to system call */
#define	SIGPIPE	13	/* write on a pipe with no one to read it */
#define	SIGALRM	14	/* alarm clock */
#define	SIGTERM	15	/* software termination signal from kill */
#define	SIGURG	16	/* urgent condition on IO channel */
#define	SIGSTOP	17	/* sendable stop signal not from tty */
#define	SIGTSTP	18	/* stop signal from tty */
#define	SIGCONT	19	/* continue a stopped process */
#define	SIGCHLD	20	/* to parent on child stop or exit */
#define	SIGCLD	20	/* System V name for SIGCHLD */
#define	SIGTTIN	21	/* to readers pgrp upon background tty read */
#define	SIGTTOU	22	/* like TTIN for output if (tp->t_local&LTOSTOP) */
#define	SIGIO	23	/* input/output possible signal */
#define	SIGPOLL	SIGIO	/* System V name for SIGIO */
#define	SIGXCPU	24	/* exceeded CPU time limit */
#define	SIGXFSZ	25	/* exceeded file size limit */
#define	SIGVTALRM 26	/* virtual time alarm */
#define	SIGPROF	27	/* profiling time alarm */
#define	SIGWINCH 28	/* window changed */
#define	SIGLOST 29	/* resource lost (eg, record-lock lost) */
#define SIGUSR1 30	/* user defined signal 1 */
#define SIGUSR2 31	/* user defined signal 2 */

#ifndef KERNEL
int	(*signal())();
#endif !KERNEL

#ifndef LOCORE
/*
 * Signal vector "template" used in sigvec call.
 */
struct	sigvec {
	int	(*sv_handler)();	/* signal handler */
	int	sv_mask;		/* signal mask to apply */
	int	sv_flags;		/* see signal options below */
};
#define SV_ONSTACK	0x0001	/* take signal on signal stack */
#define SV_INTERRUPT	0x0002	/* do not restart system on signal return */
#define SV_RESETHAND	0x0004	/* reset signal handler to SIG_DFL when signal taken */
#define sv_onstack sv_flags	/* isn't compatibility wonderful! */

/*
 * Structure used in sigstack call.
 */
struct	sigstack {
	char	*ss_sp;			/* signal stack pointer */
	int	ss_onstack;		/* current status */
};

/*
 * Information pushed on stack when a signal is delivered.
 * This is used by the kernel to restore state following
 * execution of the signal handler.  It is also made available
 * to the handler to allow it to properly restore state if
 * a non-standard exit is performed.
 */
struct	sigcontext {
	int	sc_onstack;		/* sigstack state to restore */
	int	sc_mask;		/* signal mask to restore */
	int	sc_sp;			/* sp to restore */
	int	sc_pc;			/* pc to retore */
	int	sc_ps;			/* psl to restore */
};
#endif !LOCORE

#define	BADSIG		(int (*)())-1
#define	SIG_DFL		(int (*)())0
#define	SIG_IGN		(int (*)())1

#ifdef KERNEL
#define	SIG_CATCH	(int (*)())2
#define	SIG_HOLD	(int (*)())3
#endif KERNEL
#endif !NSIG

/*
 * Macro for converting signal number to a mask suitable for sigblock().
 */
#define sigmask(m)	(1 << ((m)-1))
