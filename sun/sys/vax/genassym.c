#ifndef lint
static	char sccsid[] = "@(#)genassym.c 1.1 86/09/25 SMI"; /* from UCB */
#endif

#define	VAX780	1
#define	VAX750	1
#define	VAX730	1

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/buf.h"
#include "../h/vmmeter.h"
#include "../h/vmparam.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/cmap.h"
#include "../h/map.h"
#include "../vaxuba/ubareg.h"
#include "../vaxuba/ubavar.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../vax/rpb.h"
#include "../h/mbuf.h"
#include "../h/msgbuf.h"

main()
{
	register struct proc *p = (struct proc *)0;
	register struct uba_regs *uba = (struct uba_regs *)0;
	register struct uba_hd *uh = (struct uba_hd *)0;
	register struct vmmeter *vm = (struct vmmeter *)0;
	register struct user *up = (struct user *)0;
	register struct rusage *rup = (struct rusage *)0;
	struct rpb *rp = (struct rpb *)0;
	struct text *tp = (struct text *)0;

	printf("#ifdef LOCORE\n");
	printf("#define\tP_LINK %d\n", &p->p_link);
	printf("#define\tP_RLINK %d\n", &p->p_rlink);
	printf("#define\tP_XLINK %d\n", &p->p_xlink);
	printf("#define\tP_ADDR %d\n", &p->p_addr);
	printf("#define\tP_PRI %d\n", &p->p_pri);
	printf("#define\tP_STAT %d\n", &p->p_stat);
	printf("#define\tP_WCHAN %d\n", &p->p_wchan);
	printf("#define\tP_TSIZE %d\n", &p->p_tsize);
	printf("#define\tP_SSIZE %d\n", &p->p_ssize);
	printf("#define\tP_P0BR %d\n", &p->p_p0br);
	printf("#define\tP_P1BR %d\n", &p->p_p1br);
	printf("#define\tP_SZPT %d\n", &p->p_szpt);
	printf("#define\tP_TEXTP %d\n", &p->p_textp);
	printf("#define\tP_FLAG %d\n", &p->p_flag);
	printf("#define\tSSLEEP %d\n", SSLEEP);
	printf("#define\tSRUN %d\n", SRUN);
	printf("#define\tUBA_BRRVR %d\n", uba->uba_brrvr);
	printf("#define\tUH_UBA %d\n", &uh->uh_uba);
	printf("#define\tUH_VEC %d\n", &uh->uh_vec);
	printf("#define\tUH_SIZE %d\n", sizeof (struct uba_hd));
	printf("#define\tRP_FLAG %d\n", &rp->rp_flag);
	printf("#define\tX_CADDR %d\n", &tp->x_caddr);
	printf("#define\tV_SWTCH %d\n", &vm->v_swtch);
	printf("#define\tV_TRAP %d\n", &vm->v_trap);
	printf("#define\tV_SYSCALL %d\n", &vm->v_syscall);
	printf("#define\tV_INTR %d\n", &vm->v_intr);
	printf("#define\tV_PDMA %d\n", &vm->v_pdma);
	printf("#define\tV_FAULTS %d\n", &vm->v_faults);
	printf("#define\tV_PGREC %d\n", &vm->v_pgrec);
	printf("#define\tV_FASTPGREC %d\n", &vm->v_fastpgrec);
	printf("#define\tNBPG %d\n", NBPG);
	printf("#define\tPGSHIFT %d\n", PGSHIFT);
	printf("#define\tUPAGES %d\n", UPAGES);
	printf("#define\tCLSIZE %d\n", CLSIZE);
	printf("#define\tSYSPTSIZE %d\n", SYSPTSIZE);
	printf("#define\tUSRPTSIZE %d\n", USRPTSIZE);
	printf("#define\tUSRSTACK %d\n", USRSTACK);
	printf("#define\tP1PAGES %d\n", P1PAGES);
	printf("#define\tHIGHPAGES %d\n", HIGHPAGES);
	printf("#define\tKERNSTACK %d\n", KERNSTACK);
	printf("#define\tKERNELBASE 0x%x\n", KERNELBASE);
	printf("#define\tUADDR 0x%x\n", UADDR);
	printf("#define\tMSGBUFPTECNT %d\n", btoc(sizeof (struct msgbuf)));
	printf("#define\tNMBCLUSTERS %d\n", NMBCLUSTERS);
	printf("#define\tU_PROCP %d\n", &up->u_procp);
	printf("#define\tU_RU %d\n", &up->u_ru);
	printf("#define\tRU_MINFLT %d\n", &rup->ru_minflt);
	printf("#define\tPCB_KSP %d\n", &up->u_pcb.pcb_ksp);
	printf("#define\tPCB_ESP %d\n", &up->u_pcb.pcb_esp);
	printf("#define\tPCB_SSP %d\n", &up->u_pcb.pcb_ssp);
	printf("#define\tPCB_USP %d\n", &up->u_pcb.pcb_usp);
	printf("#define\tPCB_P0BR %d\n", &up->u_pcb.pcb_p0br);
	printf("#define\tPCB_P0LR %d\n", &up->u_pcb.pcb_p0lr);
	printf("#define\tPCB_P1BR %d\n", &up->u_pcb.pcb_p1br);
	printf("#define\tPCB_P1LR %d\n", &up->u_pcb.pcb_p1lr);
	printf("#define\tPCB_SZPT %d\n", &up->u_pcb.pcb_szpt);
	printf("#define\tPCB_R11 %d\n", &up->u_pcb.pcb_r11);
	printf("#define\tPCB_PC %d\n", &up->u_pcb.pcb_pc);
	printf("#define\tPCB_PSL %d\n", &up->u_pcb.pcb_psl);
	printf("#define\tPCB_SIGC %d\n", up->u_pcb.pcb_sigc);
	printf("#define\tPCB_CMAP2 %d\n", &up->u_pcb.pcb_cmap2);
	printf("#define\tPCB_SSWAP %d\n", &up->u_pcb.pcb_sswap);
	printf("#else\n");
	printf("asm(\".set\tU_ARG,%d\");\n", up->u_arg);
	printf("asm(\".set\tU_QSAVE,%d\");\n", up->u_qsave);
	printf("#endif\n");
}
