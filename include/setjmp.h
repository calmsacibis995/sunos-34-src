/*	@(#)setjmp.h 1.1 86/09/24 SMI; from UCB 4.1 83/05/03	*/

#ifdef vax
typedef int jmp_buf[10];
#endif

#ifdef mc68000
typedef int jmp_buf[15];	/* pc,sigmask,onsstack,d2-7,a2-7 */
#endif
