/*	@(#) buserr.h 1.1 9/25/86 Copyright Sun Microsystems Inc	*/
struct buserrinfo {
	u_long	b_a0, b_a1, b_d0, b_d1,		/* saved register info */
		b_sr, b_pc, b_vss, b_fault;	/* 68010 info */
};

extern jmp_buf		buf_buserr;		/* used in longjmp if */
extern int		use_buserrbuf;		/* non-zero */
extern int		buserr();		/* used to declare vector */
