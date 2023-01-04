#ifndef lint
static	char sccsid[] = "@(#)Locore.c 1.1 86/09/25";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/vm.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/msgbuf.h"
#include "../h/mbuf.h"
#include "../h/protosw.h"
#include "../h/domain.h"
#include "../h/uio.h"
#include "../h/socket.h"

#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/reg.h"
#include "../machine/scb.h"
#include "../machine/buserr.h"

#include "../sundev/mbvar.h"
#include "../sundev/zsreg.h"

#include "../net/if.h"
#include "../netinet/in.h"
#include "../netinet/if_ether.h"

#ifdef NFS
#include "../rpc/types.h"
#include "../rpc/xdr.h"
#endif NFS

/*
 * Pseudo file for lint to show what is used/defined
 * in locore.s and other such files.
 */
 
int	intstack[1];		/* the interrupt stack */
 
#ifdef notdef
struct	user u;
#endif
struct	scb scb;
struct	scb protoscb;
struct	msgbuf msgbuf;

typedef	int (*func)();
extern func *vector[7];

struct	proc *masterprocp;	/* proc pointer for current process mapped */
struct	proc *fpprocp;		/* last proc to have external state for fpp */
short	fppstate = 1;		/* 0 = no 81, 1 = 81 enable, -1 = 81 disabled */
u_char	enablereg = 0;		/* software copy of system enable register */

lowinit()
{
	struct regs regs;
	struct stkfmt stkfmt;
	extern char *strcpy();
	extern int (*caller())();
	extern int (*callee())();
	extern long dumpmag;
	extern int nkeytables;
	extern struct domain unixdomain;
#ifdef PUP
	extern struct domain pupdomain;
#endif
#ifdef INET
	extern struct domain inetdomain;
#endif
#include "imp.h"
#if NIMP > 0
	extern struct domain impdomain;
#endif
#ifdef NIT
	extern struct domain nitdomain;
#endif

	/* cpp messes these up for lint so put them here */
	unixdomain.dom_next = domains;
	domains = &unixdomain;
#ifdef PUP
	pupdomain.dom_next = domains;
	domains = &pupdomain;
#endif
#ifdef INET
	inetdomain.dom_next = domains;
	domains = &inetdomain;
#endif
#if NIMP > 0
	impdomain.dom_next = domains;
	domains = &impdomain;
#endif
#if NIT
	nitdomain.dom_next = domains;
	domains = &nitdomain;
#endif

	dumpmag = 0;			/* used only by savecore */

	/*
	 * Pseudo-uses of globals.
	 */
	lowinit();
	intstack[0] = intstack[0];
	scb = scb;
	protoscb = protoscb;
	vector[0] = vector[0];
	enablereg = enablereg;
#ifdef notdef
	u = u;
#endif
	(void) main(regs);
	tracedump();
	dumpsys();

	/*
	 * Routines called from interrupt vectors.
	 */
	panic("Machine check");
	printf("Write timeout");
	hardclock((caddr_t)0, 0);
	softint();
	(void) trap(0, regs, stkfmt);
	syscall(0, regs);
	ipintr();
	rawintr();
#include "nd.h"
#if NND > 0
	ndintr();
#endif
#include "fpa.h"
#if NFPA > 0
	fpa_fix_bestack((struct bei_longb *)0);
#endif

	if (vmemall((struct pte *)0, 0, (struct proc *)0, 0))
		return;		/* use value */
	memerr();
	boothowto = 0;

	(void) spl0();
	(void) spl1();
	(void) spl2();
	(void) spl3();
	(void) spl4();
	(void) spl5();
	(void) spl6();
	(void) spl7();
	(void) splzs();
	(void) splie();
	(void) splnet();
	(void) splimp();
	(void) splsoftclock();

	/*
	 * Misc uses for some things which
	 * aren't neccesarily used.
	 */
	harderr((struct buf *)0, "bogus");
	ip_ctlinput(0, (caddr_t)0);
	(void) strcpy((char *)0, (char *)0);
	(void) caller();
	(void) callee();
	nullsys();
	nkeytables = nkeytables;

	setusercontext(getusercontext());
	add_default_intr((func)0);
	{
		extern char CADDR2[];

		CADDR2[0] = CADDR2[0];
		CMAP2[0] = CMAP2[0];
	}
	(void) mballoc(&mb_hd, (caddr_t)0, 0, 0);
	prtmsgbuf();
	errsys();
	(void) m_cpytoc((struct mbuf *)0, 0, 0, (caddr_t)0);
	(void) m_cappend((char *)0, 0, (struct mbuf *)0);
#ifdef NFS
	(void) xdr_netobj((XDR *)0, (struct netobj *)0);
#endif NFS
#ifdef SUN3_260
	vac_enable_kpage((caddr_t)0);
#endif SUN3_260
}

char	DVMA[0x1000000];

struct	pte Sysmap[SYSPTSIZE];
struct	pte Usrptmap[USRPTSIZE];
struct	pte usrpt[USRPTSIZE*NPTEPG];
struct	pte Forkmap[UPAGES];
struct	user forkutl;
struct	pte Xswapmap[UPAGES];
struct	user xswaputl;
struct	pte Xswap2map[UPAGES];
struct	user xswap2utl;
struct	pte Swapmap[UPAGES];
struct	user swaputl;
struct	pte Pushmap[UPAGES];
struct	user pushutl;
struct	pte Vfmap[UPAGES];
struct	user vfutl;
struct	pte CMAP1[1];
char	CADDR1[NBPG];
struct	pte CMAP2[1];
char	CADDR2[NBPG];
struct	pte mmap[1];
char	vmmap[NBPG];
struct	pte Mbmap[NMBCLUSTERS/CLSIZE];
struct	mbuf mbutl[NMBCLUSTERS*CLBYTES/sizeof (struct mbuf)];
struct	pte ESysmap[1];

level7() { ; }

/*ARGSUSED*/
copyin(udaddr, kaddr, n) caddr_t udaddr, kaddr; u_int n; { return (0); }

/*ARGSUSED*/
copyout(kaddr, udaddr, n) caddr_t kaddr, udaddr; u_int n; { return (0); }

/*ARGSUSED*/
fubyte(base) caddr_t base; { return (0); }

/*ARGSUSED*/
fuibyte(base) caddr_t base; { return (0); }

/*ARGSUSED*/
subyte(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
suibyte(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
fuword(base) caddr_t base; { return (0); }

/*ARGSUSED*/
fuiword(base) caddr_t base; { return (0); }

/*ARGSUSED*/
suword(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
suiword(base, i) caddr_t base; { return (0); }

/*ARGSUSED*/
copyinstr(udaddr, kaddr, maxlength, lencopied)
caddr_t udaddr, kaddr; u_int maxlength, *lencopied; { return (0); }

/*ARGSUSED*/
copyoutstr(kaddr, udaddr, maxlength, lencopied)
caddr_t kaddr, udaddr; u_int maxlength, *lencopied; { return (0); }

/*ARGSUSED*/
copystr(kfaddr, kdaddr, maxlength, lencopied)
caddr_t kfaddr, kdaddr; u_int maxlength, *lencopied; { return (0); }

/*ARGSUSED*/
montrap(f) void (*f)(); { ; }

siron() { ; }

intsvc() { return (0); }

/*ARGSUSED*/
setvideoenable(s) u_int s; { ; }

/*ARGSUSED*/
setcopyenable(s) u_int s; { ; }

/*ARGSUSED*/
setintrenable(s) u_int s; { ; }

/*ARGSUSED*/
getidprom(s) char *s; { ; }

enable_dvma() { ; }

disable_dvma() { ; }

/*ARGSUSED*/
fulwds(uadd, sadd, nlwds) caddr_t uadd, sadd; int nlwds; { return (0); }

struct scb *getvbr() { return ((struct scb *)0); }

/*ARGSUSED*/
setvbr(scbp) struct scb *scbp; { ; }

getbuserr() { return (0); }

setfppregs() { ; }

/*ARGSUSED*/
on_enablereg(reg_bit) u_char reg_bit; { ; }

/*ARGSUSED*/
off_enablereg(reg_bit) u_char reg_bit; { ; } 

/*
 * Routines from movc.s
 */
/*ARGSUSED*/
bcopy(from, to, count) caddr_t from, to; u_int count; { ; }

/*ARGSUSED*/
ovbcopy(from, to, count) caddr_t from, to; u_int count; { ; }

/*ARGSUSED*/
bzero(base, count) caddr_t base; u_int count; { ; }

/*ARGSUSED*/
blkclr(base, count) caddr_t base; u_int count; { ; }

/*
 * From addupc.s
 */
/*ARGSUSED*/
addupc(pc, pr, incr) int pc; struct uprof *pr; int incr; { ; }

/*
 * From ocsum.s
 */
/*ARGSUSED*/
ocsum(p, len) u_short *p; int len; { return (0); }

/*
 * Routines from setjmp.s
 */
/*ARGSUSED*/
setjmp(lp) label_t *lp; { return (0); }

/*ARGSUSED*/
longjmp(lp) label_t *lp; { /*NOTREACHED*/ }

/*ARGSUSED*/
syscall_setjmp(lp) label_t *lp; { return (0); }


/*
 * Routines from vax.s
 */
/*ARGSUSED*/
splx(s) int s; { return (0); }

/*ARGSUSED*/
splr(s) int s; { return (0); }

spl0() { return (0); }
spl1() { return (0); }
spl2() { return (0); }
spl3() { return (0); }
spl4() { return (0); }
spl5() { return (0); }
spl6() { return (0); }
spl7() { return (0); }
splzs() { return (0); }
splie() { return (0); }
splnet() { return (0); }
splimp() { return (0); }
splclock() { return (0); }
splsoftclock() { return (0); }

/*ARGSUSED*/
setrq(p) struct proc *p; { ; }

/*ARGSUSED*/
remrq(p) struct proc *p; { ; }

swtch() { if (whichqs) whichqs = 0; }

/*ARGSUSED*/
resume(p) struct proc *p; { if (masterprocp) masterprocp = p; }

/*ARGSUSED*/
_remque(p) caddr_t p; { ; }

/*ARGSUSED*/
_insque(out, in) caddr_t out, in;  { ; }

/*ARGSUSED*/
scanc(size, cp, table, mask)
u_int size; caddr_t cp, table; int mask; { return (0); }


/*
 * Routines from map.s
 */
/*ARGSUSED*/
unloadpgmap(pageno, pte, count) u_int pageno; struct pte* pte; int count; { ; }

/*ARGSUSED*/
long getpgmap(v) caddr_t v; { return ((long)0); }

/*ARGSUSED*/
setpgmap(v, pte) caddr_t v; long pte; { ; }

/*ARGSUSED*/
loadpgmap(pageno, pte, new, count) 
u_int pageno; struct pte *pte; int new, count; { ; }

/*ARGSUSED*/
u_char getsegmap(segno) u_int segno; { return ((u_char)0); }

/*ARGSUSED*/
setsegmap(segno, pm) u_int segno; u_char pm; { ; }

getcontext() { return (0); }

/*ARGSUSED*/
setcontext(uc) int uc; { ; }

/*ARGSUSED*/
vac_init() { ; }

/*ARGSUSED*/
vac_ctxflush() { ; }

/*ARGSUSED*/
vac_segflush(segno) u_int segno; { ; }

/*ARGSUSED*/
vac_pageflush(vaddr) caddr_t vaddr; { ; }

/*ARGSUSED*/
vac_flush(vaddr, nbytes) caddr_t vaddr; int nbytes; { ; }

/*
 * getusercontext() and setusercontext are aliases
 * for getcontext() and setcontext() respectively
 */
getusercontext() { return (0); }

/*ARGSUSED*/
setusercontext(uc) int uc; { ; }

#ifdef PUP
/*
 * Needs to be actually written if PUP is ever really used on a Sun
 */
/*ARGSUSED*/
pup_cksum(m, len) struct mbuf *m; int len; { return (0); }
#endif PUP

/*
 * From zs_asm.s
 */
#include "zs.h"
int (*zsvectab[NZS][8])();

setzssoft() { ; }

clrzssoft() { return (0); }

/*ARGSUSED*/
zszread(zsaddr, n) struct zscc_device *zsaddr; int n; { return (0); }

/*ARGSUSED*/
zszwrite(zsaddr, n, v) struct zscc_device *zsaddr; int n, v; { ; }

#include "gpone.h"
#if NGPONE > 0
/*
 * From gp1_kern_sync.s
 */
/*ARGSUSED*/
gp1_kern_sync(shmem) caddr_t shmem; { return (0); }
#endif NGPONE > 0

/*
 * Variables declared for savecore, or
 * implicitly, such as by config or the loader.
 */
char	version[] = "Sun UNIX 4.2 Release ...";
char	start[1];
char	etext[1];
char	end[1];
char	Syslimit[1];
