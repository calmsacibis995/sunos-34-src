#ifndef lint
static char sccsid[] = "@(#)ipc_shm.c 1.1 86/09/25 Sun Micro";	/* from S5R2 6.1 */
#endif
/*
 *	Inter-Process Communication Shared Memory Facility.
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/dir.h"
#include "../h/signal.h"
#include "../h/seg.h"
#include "../h/ipc.h"
#include "../h/shm.h"
#include "../h/proc.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"


	/* PRE-VM-REWRITE */
/* define some flags for the mmap compatibility implementation */
#define	SHM_FILENO	PG_FTEXT
#define SHM_PGFLAG	(PG_V | PG_FOD)

/* define memory allocation routines */
int	vmemall();
caddr_t	zmemall();
	/* END ... PRE-VM-REWRITE */


/* define mask for virtual address validity checking */
#define SHM_CLOFSET (shm_alignment - 1)		/* see <machine/param.h> */


struct	shmid_ds	*shmconv();
struct	ipc_perm	*ipcget();

unsigned	shmtot;		/* total shared memory in use */

/*
 *	shmconv - Convert user supplied shmid into a ptr to the associated
 *		shared memory header.
 */

struct shmid_ds *
shmconv(s)
register int	s;	/* shmid */
{
	register struct shmid_ds	*sp;	/* ptr to associated header */

        if (s < 0) {
		u.u_error = EINVAL;
		return(NULL);
	}

	sp = &shmem[s % shminfo.shmmni];
	if (!(sp->shm_perm.mode & IPC_ALLOC) ||
		s / shminfo.shmmni != sp->shm_perm.seq) {
		u.u_error = EINVAL;
		return(NULL);
	}
	return(sp);
}

/*
 *	shmctl - Shmctl system call.
 */

shmctl()
{
	register struct a {
		int		shmid;
		int		cmd;
		struct shmid_ds	*arg;
	}	*uap = (struct a *)u.u_ap;
	register struct shmid_ds	*sp;	/* shared memory header ptr */
	struct shmid_ds			ds;	/* hold area for IPC_SET */

	if ((sp = shmconv(uap->shmid)) == NULL)
		return;
	if ((uap->cmd != IPC_STAT) && (sp->shm_perm.mode & SHM_DEST)) {
		u.u_error = EINVAL;
		return;
	}

	switch(uap->cmd) {

	/* Remove shared memory identifier. */
	case IPC_RMID:
		if (u.u_uid != sp->shm_perm.uid && u.u_uid != sp->shm_perm.cuid
			&& !suser())
			return;

		SHMLOCK(sp);
		sp->shm_ctime = (time_t) time.tv_sec;
		sp->shm_perm.mode |= SHM_DEST;

		/* Change key to private so old key can be reused without
			waiting for last detach.  Only allowed accesses to
			this segment now are shmdt() and shmctl(IPC_STAT).
			All others will give bad shmid. */
		sp->shm_perm.key = IPC_PRIVATE;
		SHMUNLOCK(sp);

		if (sp->shm_nattch == 0)	/* if no attachments */
			shmfree(sp);
		break;

	/* Set ownership and permissions. */
	case IPC_SET:
		if(u.u_uid != sp->shm_perm.uid && u.u_uid != sp->shm_perm.cuid
			&& !suser())
			return;
		if (u.u_error =
		    copyin((caddr_t) uap->arg, (caddr_t) &ds, sizeof(ds))) {
			return;
		}
		sp->shm_perm.uid = ds.shm_perm.uid;
		sp->shm_perm.gid = ds.shm_perm.gid;
		sp->shm_perm.mode = (ds.shm_perm.mode & 0777) |
			(sp->shm_perm.mode & ~0777);
		sp->shm_ctime = (time_t) time.tv_sec;
		break;

	/* Get shared memory data structure. */
	case IPC_STAT:
		if (ipcaccess(&sp->shm_perm, SHM_R))
			return;
		if (u.u_error =
		    copyout((caddr_t) sp, (caddr_t) uap->arg, sizeof(*sp))) {
			return;
		}
		break;
	
	/* Lock shared segment in memory. */	/* NOTE: no-op for now */
	case SHM_LOCK:
		if (!suser())
			return;
		break;

	/* Unlock shared segment. */		/* NOTE: no-op for now */
	case SHM_UNLOCK:
		if (!suser())
			return;
		break;

	default:
		u.u_error = EINVAL;
		return;
	}
}

/*
 *	shmget - Shmget system call.
 */

shmget()
{
	register struct a {
		key_t	key;
		uint	size;
		int	shmflg;
	}	*uap = (struct a *)u.u_ap;
	register struct shmid_ds	*sp;	/* shared memory header ptr */
	int				s;	/* ipcget status */
	int	size;

	if ((sp = (struct shmid_ds *) ipcget(uap->key, uap->shmflg,
		&shmem[0].shm_perm, shminfo.shmmni, sizeof(*sp), &s)) == NULL)
		return;

	if (s) {
		/* This is a new shared memory segment.  Allocate memory and
			finish initialization. */
		if (uap->size < shminfo.shmmin || uap->size > shminfo.shmmax) {
			u.u_error = EINVAL;
			sp->shm_perm.mode = 0;
			return;
		}

		size = btoc(uap->size);
		if (shmtot + size > shminfo.shmall) {
			u.u_error = ENOMEM;
			sp->shm_perm.mode = 0;
			return;
		}
		sp->shm_segsz = uap->size;
		shmtot += size;

		sp->shm_perm.mode |= SHM_INIT;
		sp->shm_lpid = 0;
		sp->shm_cpid = u.u_procp->p_pid;
		sp->shm_nattch = 0;
		sp->shm_ctime = (time_t) time.tv_sec;
		sp->shm_atime = (time_t) 0;
		sp->shm_dtime = (time_t) 0;
	} else
		if (uap->size && sp->shm_segsz < uap->size) {
			u.u_error = EINVAL;
			return;
		}
	u.u_rval1 = sp->shm_perm.seq * shminfo.shmmni + (sp - shmem);
}

/*
 *	shmat - attach a shared memory segment
 */

shmat()
{
	register struct a {
		int	shmid;
		uint	addr;
		int	flag;
	}	*uap = (struct a *)u.u_ap;
	register struct shmid_ds	*sp;	/* shared memory header ptr */
	register struct shmid_ds	**spp;
	register int	shmn;
	int		off;
	uint		size;
	uint		fv, lv, pm;
	struct pte	*pte;

#ifdef notdef
	sysinfo.shm++;                  /* bump shared memory count */
#endif notdef

	if ((sp = shmconv(uap->shmid)) == NULL)
		return;
	if (sp->shm_perm.mode & SHM_DEST) {
		u.u_error = EINVAL;
		return;
	}
	if (ipcaccess(&sp->shm_perm, SHM_R))
		return;
	if ((uap->flag & SHM_RDONLY) == 0)
		if (ipcaccess(&sp->shm_perm, SHM_W))
			return;

	/* PRE-VM-REWRITE */
	/* address 0 should be filtered at the syscall library */
	if (uap->addr == 0) {
		printf("shmat called with 0 address?\n");
		u.u_error = EINVAL;
		return;
	}
	/* END ... PRE-VM-REWRITE */

	if (uap->flag & SHM_RND)
		uap->addr &= ~(SHMLBA - 1);
	if (uap->addr & SHM_CLOFSET) {
		u.u_error = EINVAL;
		return;
	}

	/* PRE-VM-REWRITE */
	/* look for a slot in the per-process shm list */
	spp = &shm_shmem[(u.u_procp - proc) * shminfo.shmseg];
	for (shmn = 0; shmn < shminfo.shmseg; shmn++, spp++)
		if (*spp == NULL)
			break;
	if (shmn >= shminfo.shmseg) {
		u.u_error = EMFILE;
		return;
	}

	/* NOTE: much of the following is ripped-off from kern_mman.c */
	/* NOTE: the rest is ripped-off from Sun consulting shm driver */

	/* make sure that the target address range is already mapped */
	size = btoc(sp->shm_segsz);
	fv = btop(uap->addr);
	lv = fv + size - 1;
	if ( (lv < fv) || !isadsv(u.u_procp, fv) || !isadsv(u.u_procp, lv) ) {
		printf("shmat called with unmapped address range %x - %x\n",
					ctob(fv), (ctob(lv+1) - 1));
		u.u_error = EINVAL;
		return;
	}

	/* If first attach, create the shared memory region */
	SHMLOCK(sp);
	if (sp->shm_perm.mode & SHM_INIT) {
		if ((sp->shm_kaddr =
			    (uint)zmemall(vmemall, (int)ctob(size))) == 0) {
			SHMUNLOCK(sp);
			u.u_error = ENOMEM;
			return;
		}
		sp->shm_perm.mode &= ~SHM_INIT;
	}
	SHMUNLOCK(sp);

	pm = SHM_PGFLAG | ((uap->flag & SHM_RDONLY) ? PG_URKR : PG_UW);

	/* flush the cache before changing the mapping */
	vac_flush((caddr_t)uap->addr, sp->shm_segsz);
	for (off = 0; off < size; off++) {
		pte = vtopte(u.u_procp, (fv + off));
		u.u_procp->p_rssize -= vmemfree(pte, 1);
		*(int *)pte = pm | (getkpgmap((caddr_t)
				    (sp->shm_kaddr + ctob(off))) & PG_PFNUM);
		((struct fpte *)pte)->pg_fileno = SHM_FILENO;
	}
	newptes(vtopte(u.u_procp, fv), fv, (int)size);
	*spp = sp;

	/* END ... PRE-VM-REWRITE */

	sp->shm_nattch++;
	sp->shm_atime = (time_t) time.tv_sec;
	sp->shm_lpid = u.u_procp->p_pid;

	u.u_rval1 = uap->addr;
}


/*
 *	shmdt - Shmdt system call.
 */

shmdt()
{
	struct a {
		uint	addr;
	}	*uap = (struct a *)u.u_ap;
	register struct shmid_ds	*sp;
	register struct shmid_ds	**spp;
	register int	i;
	struct pte *	pte;

/*
 * Check for page alignment
 */
	if ( (uap->addr == 0) || (uap->addr & SHM_CLOFSET) ) {
		u.u_error = EINVAL;
		return;
	}
/*
 * find segment
 */
	/* PRE-VM-REWRITE */
	pte = vtopte(u.u_procp, btop(uap->addr));

	/* check that user address is mapped to a shared memory segment */
	if ( (((struct fpte *)pte)->pg_fileno != SHM_FILENO) ||
			    (((*(int *)pte) & SHM_PGFLAG) != SHM_PGFLAG) ) {
		u.u_error = EINVAL;
		return;
	}

	/* look for attached shared memory with same kernel page */
	spp = &shm_shmem[(u.u_procp - proc) * shminfo.shmseg];
	for (i = 0; i < shminfo.shmseg; i++, spp++) {
		if ( ((sp = *spp) != NULL) &&
		    (pte->pg_pfnum ==
			    (getkpgmap((caddr_t)sp->shm_kaddr) & PG_PFNUM)) ) {
			break;
		}
		sp = NULL;
	}
	if (sp == NULL) {
		u.u_error = EINVAL;
		return;
	}
	*spp = NULL;
	shmunmap(sp, uap->addr);
	/* END ... PRE-VM-REWRITE */

	sp->shm_dtime = (time_t) time.tv_sec;
	sp->shm_lpid = u.u_procp->p_pid;
}

/*
 *	shmsys - System entry point for shmat, shmctl, shmdt, and shmget
 *		system calls.
 */

shmsys()
{
	register struct a {
		uint	id;
	}	*uap = (struct a *)u.u_ap;
	int		shmat(),
			shmctl(),
			shmdt(),
			shmget();
	/* PRE-VM-REWRITE */
	int		shmalignment();
	static int	(*calls[])() = {shmat, shmctl, shmdt, shmget
							    , shmalignment };
	/* END ... PRE-VM-REWRITE */

	if(uap->id > 4) {
		u.u_error = EINVAL;
		return;
	}
	u.u_ap = &u.u_arg[1];
	(*calls[uap->id])();
}

	/* PRE-VM-REWRITE */
/*
 *	shmalignment - return the current system's shared memory alignment
 *	restrictions.
 */
int
shmalignment()
{
	u.u_rval1 = shm_alignment;	/* macro defined in <machine/param.h> */
}


/*
 *	shmunmap - unmap the current task's shared memory.
 *		   Calls shmfree() to delete region if necessary.
 *		   NOTE: this code is ripped-off from kern_mman.c
 */

shmunmap(sp, addr)
register struct shmid_ds	*sp;
uint	addr;
{
	register struct pte	*pte;
	uint		off, size;
	uint		fv;

	fv = btop(addr);
	size = btoc(sp->shm_segsz);

	/* flush the cache before changing the mapping */
	vac_flush((caddr_t)addr, sp->shm_segsz);

	for (off = 0; off < size; off++) {
		pte = vtopte(u.u_procp, (fv + off));

		/* sanity check */
		if ( (((struct fpte *)pte)->pg_fileno != SHM_FILENO) ||
			    (((*(int *)pte) & SHM_PGFLAG) != SHM_PGFLAG) )
			panic("shmunmap mapping violation\n");

		u.u_procp->p_rssize -= vmemfree(pte, 1);
		*(int *)pte = (PG_UW | PG_FOD);
		((struct fpte *)pte)->pg_fileno = PG_FZERO;
	}
	newptes(vtopte(u.u_procp, fv), fv, (int)size);

	if ( (--(sp->shm_nattch) == 0) && (sp->shm_perm.mode & SHM_DEST) )
		shmfree(sp);
}

/*
 *	shmfree - Free shared memory segment.
 */

shmfree(sp)
register struct shmid_ds	*sp;	/* shared memory header ptr */
{
	register int size;

	size = btoc(sp->shm_segsz);
	shmtot -= size;

	if ((sp->shm_perm.mode & SHM_INIT) == 0)
	    wmemfree((caddr_t)sp->shm_kaddr, ctob(size));

	sp->shm_perm.mode = 0;
	if (((int)(++(sp->shm_perm.seq)*shminfo.shmmni + (sp - shmem))) < 0)
		sp->shm_perm.seq = 0;
}

/*
 *	shmfork - Called by newproc (kern_fork.c) to handle shared memory fork
 *		processing.
 */

shmfork(cp, pp)
struct proc	*cp;	/* ptr to child proc table entry */
struct proc	*pp;	/* ptr to parent proc table entry */
{
	register struct shmid_ds	**cpp,	/* ptr to child shmem ptrs */
					**ppp;	/* ptr to parent shmem ptrs */
	register int			i;	/* loop control */

	/* Update counts on any attached segments. */
	cpp = &shm_shmem[(cp - proc) * shminfo.shmseg];
	ppp = &shm_shmem[(pp - proc) * shminfo.shmseg];

	for (i = 0; i < shminfo.shmseg; i++, cpp++, ppp++) {
		if (*cpp = *ppp) {
			(*cpp)->shm_nattch++;
		}
	}
}

/*
 *	shmexec - Called by getxfile (kern_exec.c) to handle shared memory exec
 *		processing.
 */

shmexec()
{
	register struct shmid_ds	**spp;	/* ptr to ptr to header */
	register struct shmid_ds	*sp;	/* ptr to header */
	register int			i;	/* loop control */

	/* Detach any attached segments. */
	spp = &shm_shmem[(u.u_procp - proc) * shminfo.shmseg];
	for (i = 0; i < shminfo.shmseg; i++, spp++) {
		if ((sp = *spp) == NULL)
			continue;
		*spp = NULL;
		if ( (--(sp->shm_nattch) == 0) &&
					    (sp->shm_perm.mode & SHM_DEST) )
			shmfree(sp);
	}
}

/*
 *	shmexit - Called by exit (kern_exit.c) to clean up on process exit.
 */

shmexit()
{
	/* Same processing as for exec. */
	shmexec();
}
	/* END ... PRE-VM-REWRITE */
