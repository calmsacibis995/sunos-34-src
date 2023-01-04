#ifndef lint
static	char sccsid[] = "@(#)gcore.c 1.1 86/09/25 SMI"; /* from UCB 4.3 05/22/83 */
#endif

/*
 * gcore - get core images of running processes
 *
 * Author: Eric Cooper
 * Written: Fall 1981.
 *
 * Inspired by a version 6 program by Len Levin, 1978.
 * Several pieces of code lifted from Bill Joy's 4BSD ps.
 *
 * Permission to copy or modify this program in whole or in part is hereby
 * granted, provided that the above credits are preserved.
 *
 * This code performs a simple simulation of the virtual memory system in user
 * code.  If the virtual memory system changes, this program must be modified
 * accordingly.  It must also be recompiled whenever system data structures
 * change.
 */
#include <stdio.h>
#include <nlist.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <machine/pte.h>
#include <sys/vm.h>
#include <setjmp.h>
#include <sys/core.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sundev/fpareg.h>

/* Various macros for efficiency. */

#define	min(a, b)	(a < b ? a : b)

#define	Seek(f, pos) {\
	if (lseek(f, (long) (pos), 0) != (long) (pos)) \
		panic("seek error"); \
}

#define	Read(f, addr, n) {\
	if (read(f, (char *) (addr), (int) (n)) != (int) (n)) \
		panic("read error"); \
}

#define	Get(f, pos, addr, n) {\
	Seek(f, pos); \
	Read(f, addr, n); \
}

struct	nlist nl[] = {
	{ "_proc" },
#define	X_PROC		0
	{ "_Usrptmap" },
#define	X_USRPTMA	1
	{ "_usrpt" },
#define	X_USRPT		2
	{ "_nswap" },
#define	X_NSWAP		3
	{ "_nproc" },
#define	X_NPROC		4
	{ "_dmmin" },
#define	X_DMMIN		5
	{ "_dmmax" },
#define	X_DMMAX		6
	{ 0 },
};

#define FEW	20		/* for fewer system calls */
struct	proc proc[FEW];

union {
	struct user user;
	char upages[UPAGES][NBPG];
} user;
#define u	user.user
#define uarea	user.upages

#define NLIST	"/vmunix"
#define KMEM	"/dev/kmem"
#define MEM	"/dev/mem"
#define SWAP	"/dev/drum"	/* "/dev/swap" on some systems */

int	nproc;
int	nswap;
int	dmmin, dmmax;
struct	pte *Usrptmap, *usrpt;
char	coref[20];
int	kmem, mem, swap, cor;
jmp_buf	cont_frame;

main(argc, argv)
	int argc;
	char **argv;
{
	register int i, j;
	register struct proc *p;
	off_t procbase, procp;
	int pid, uid;
	char c;

	if (argc < 2) {
		printf("Usage: %s pid ...\n", argv[0]);
		exit(1);
	}
	openfiles();
	getkvars();
	procbase = getw(nl[X_PROC].n_value);
	nproc = getw(nl[X_NPROC].n_value);
	nswap = getw(nl[X_NSWAP].n_value);
	dmmin = getw(nl[X_DMMIN].n_value);
	dmmax = getw(nl[X_DMMAX].n_value);
	while (--argc > 0) {
		if ((pid = atoi(*++argv)) <= 0 || setjmp(cont_frame))
			continue;
		printf("%d: ", pid);
		procp = procbase;
		for (i = 0; i < nproc; i += FEW) {
			Seek(kmem, procp);
			j = nproc - i;
			if (j > FEW)
				j = FEW;
			j *= sizeof(struct proc);
			Read(kmem, (char *) proc, j);
			procp += j;
			for (j = j / sizeof(struct proc) - 1; j >= 0; j--) {
				p = &proc[j];
				if (p->p_pid == pid)
					goto found;
			}
		}
		printf("Process not found.\n");
		continue;
	found:
		if (p->p_uid != (uid = getuid()) && uid != 0) {
			printf("Not owner.\n");
			continue;
		}
		if (p->p_stat == SZOMB) {
			printf("Zombie.\n");
			continue;
		}
		if (p->p_flag & SWEXIT) {
			printf("Process exiting.\n");
			continue;
		}
		if (p->p_flag & SSYS) {
			printf("System process.\n");
			/* i.e. swapper or pagedaemon */
			continue;
		}
		sprintf(coref, "core.%d", pid);
		if ((cor = creat(coref, 0666)) < 0) {
			perror(coref);
			exit(1);
		}
		core(p);
		close(cor);
		printf("%s dumped\n", coref);
	}
	exit(0);
}

getw(loc)
	off_t loc;
{
	int word;

	Get(kmem, loc, &word, sizeof(int));
	return (word);
}

openfiles()
{
	kmem = open(KMEM, 0);
	if (kmem < 0) {
		perror(KMEM);
		exit(1);
	}
	mem = open(MEM, 0);
	if (mem < 0) {
		perror(MEM);
		exit(1);
	}
	swap = open(SWAP, 0);
	if (swap < 0) {
		perror(SWAP);
		exit(1);
	}
}

getkvars()
{
	nlist(NLIST, nl);
	if (nl[0].n_type == 0) {
		printf("%s: No namelist\n", NLIST);
		exit(1);
	}
	Usrptmap = (struct pte *) nl[X_USRPTMA].n_value;
	usrpt = (struct pte *) nl[X_USRPT].n_value;
}

/*
 * Get the system page table entries (mapping the user page table).
 * These are the entries Usrptmap[i .. i + szpt],
 * where i = btokmx(p->p_p0br) and szpt = p->p_szpt.
 * For our purposes, we can skip over the ptes mapping
 * the text segment ptes.
 */
struct pte	*syspt;		/* pte's from Usrptmap */
int		nsysptes;

getsyspt(p)
	register struct proc *p;
{
	nsysptes = p->p_szpt - (p->p_tsize / NPTEPG);
	syspt = (struct pte *) malloc(nsysptes * sizeof(struct pte));
	if (syspt == NULL)
		panic("can't alloc %d page table entries", nsysptes);
	Get(kmem, &Usrptmap[btokmx(p->p_p0br) + (p->p_tsize / NPTEPG)],
		syspt, nsysptes * sizeof(struct pte));
}

/*
 * Get the user page table for a segment.
 * seg 0 = p0 (not including text)
 * seg 1 = p1 (stack and u area)
 * The system pt is consulted to find each page of user ptes.
 */
struct pte *
getpt(p, seg)
	register struct proc *p;
	int seg;
{
	register int i;
	register struct pte *spt;
	struct pte *pt;
	int nptes, offset, n;

	if (seg == 0) {
		nptes = p->p_dsize;
		spt = syspt;
		offset = p->p_tsize % NPTEPG;
	} else {
		nptes = p->p_ssize + UPAGES;
		spt = syspt + (nsysptes - ctopt(nptes));
		offset = -nptes % NPTEPG;
		if (offset < 0)
			offset += NPTEPG;
	}
	pt = (struct pte *) malloc(nptes * sizeof(struct pte));
	if (pt == NULL)
		panic("can't alloc %d page table entries", nptes);
	for (i = 0; i < nptes; i += n) {
		n = min(NPTEPG - offset, nptes - i);
		Get(mem, ctob(spt->pg_pfnum) + offset * sizeof(struct pte),
		     pt + i, n * sizeof(struct pte));
		spt++;
		offset = 0;
	}
	return (pt);
}

/*
 * Build the core file.
 */
core(p)
	register struct proc *p;
{
	register struct pte *p0, *p1;

	if (p->p_flag & SLOAD) {		/* page tables are resident */
		getsyspt(p);
		p0 = getpt(p, 0);
		p1 = getpt(p, 1);
#ifdef	DEBUG
		showpt(syspt, nsysptes, "system");
		showpt(p0, p->p_dsize, "p0");
		showpt(p1, p->p_ssize + UPAGES, "p1");
#endif
	}
	getu(p, &p1[p->p_ssize]);			/* read u area */
	getseg(p, p->p_dsize, p0, &u.u_dmap, 0);	/* data */
	getseg(p, p->p_ssize, p1, &u.u_smap, 1);	/* stack */
	write(cor, uarea, ctob(UPAGES));		/* write u area */

	if (p->p_flag & SLOAD) {
		free((char *) syspt);
		free((char *) p0);
		free((char *) p1);
	}
}

/*
 * Get the u area.
 * Keeps around the u structure for later use
 * (the data and stack disk map structures).
 */
getu(p, pages)
	register struct proc *p;
	register struct pte *pages;
{
	struct 	core	core;
	register int i;
	int	regoffset;
	int	len;

	if ((p->p_flag & SLOAD) == 0) {
		Get(swap, dtob(p->p_swaddr), uarea, ctob(UPAGES));
	} else {
		for (i = 0; i < UPAGES; i += CLSIZE) {
			Get(mem, ctob(pages[i].pg_pfnum), uarea[i], 
			  ctob(CLSIZE));
		}
	}
	/*
 	 * This code is very similar to that in kern_sig.c in the
	 * kernel.  The u_ar0 member points to the place in the uarea
	 * where the registers are stored.  It must be relocated here.
	 */
	bzero((char *)&core, sizeof (core));
	regoffset = (int)u.u_ar0 - UADDR;
	core.c_magic = CORE_MAGIC;
	core.c_len = sizeof (core);
	core.c_regs = *(struct regs *)((int)&u + regoffset);
#ifdef sun
	u.u_fp_status.fps_flags = EXT_FPS_FLAGS(&u.u_fp_istate);
	u.u_fp_status.fps_code = u.u_code;
	core.c_fpstatus = u.u_fp_status;
	core.c_fparegs.fpar_flags = u.u_fpa_flags;
	core.c_fparegs.fpar_status = u.u_fpa_status;
	if (u.u_fpa_flags != 0) {
		int i;
		int fd;
		union fpa_qtr_dregs *fpadregs;
		/*
		 * core.c_fparegs.fpar_data must be set with an ioctl call.
		 * In addition, because of an ioctl restriction, we must
		 * make 4 trips to the well in order to get all 32 regs.
		 */
		fpadregs = (union fpa_qtr_dregs *)core.c_fparegs.fpar_data;
		fd = open("/dev/fpa",O_RDWR);
		if (fd == 0) {
			perror("cannot open /dev/fpa");
			exit(1);
		}
		for (i = 0; i < 4; i++) {
			fpadregs->fq_params.fqp_count = i;
			fpadregs->fq_params.fqp_state = u.u_fpa_status.fpas_state;
			if (ioctl(fd, FPA_GET_DATAREGS, fpadregs) == -1) {
				perror("cannot read fpa regs");
				break;
			}
			fpadregs++;
		}
		(void)close(fd);
	}
#else sun
	core.c_ucode = u.u_code;
#endif sun
	core.c_aouthdr = u.u_exdata.Ux_A;
	core.c_signo = u.u_arg[0];
	core.c_tsize = ctob(u.u_tsize);
	core.c_dsize = ctob(u.u_dsize);
	core.c_ssize = ctob(u.u_ssize);
	len = min(MAXCOMLEN, CORE_NAMELEN);
	strncpy(core.c_cmdname, u.u_comm, len);
	core.c_cmdname[len] = '\0';

	write(cor, (char *)&core, sizeof (core));
}

/*
 * Copy a segment to the core file.
 * The segment is described by its size in clicks,
 * its page table, its disk map, and whether or not
 * it grows backwards.
 * Note that the page table address is allowed to be meaningless
 * if the process is swapped out.
 */
getseg(p, segsize, pages, map, rev)
	register struct proc *p;
	int segsize;
	register struct pte *pages;
	struct dmap *map;
{
	register int i;
	struct dblock db;
	int size;
	char buf[ctob(CLSIZE)];

	for (i = 0; i < segsize; i += CLSIZE) {
		size = min(CLSIZE, segsize - i);
		if ((p->p_flag & SLOAD) == 0 || pages[i].pg_fod ||
		    pages[i].pg_pfnum == 0) {
			struct fpte *fpte = (struct fpte *)&pages[i];

			if (fpte->pg_fod && fpte->pg_fileno == PG_FZERO) {
				bzero(buf, ctob(size));
			} else {
				vstodb(rev ? (segsize - i - CLSIZE) : i,
				    size, map, &db, rev);
				Get(swap, dtob(db.db_base), buf, ctob(size));
			}
		} else {
			Get(mem, ctob(pages[i].pg_pfnum), buf, ctob(size));
		}
		write(cor, buf, ctob(size));
	}
}

/*
 * Given a base/size pair in virtual swap area,
 * return a physical base/size pair which is the
 * (largest) initial, physically contiguous block.
 */
vstodb(vsbase, vssize, dmp, dbp, rev)
	register int vsbase;
	int vssize;
	struct dmap *dmp;
	register struct dblock *dbp;
{
	register int blk = dmmin;
	register swblk_t *ip = dmp->dm_map;

	vsbase = ctod(vsbase);
	vssize = ctod(vssize);
	if (vsbase < 0 || vsbase + vssize > dmp->dm_size)
		panic("can't make sense out of virtual memory (gcore probably needs to be recompiled)");
	while (vsbase >= blk) {
		vsbase -= blk;
		if (blk < dmmax)
			blk *= 2;
		ip++;
	}
	if (*ip <= 0 || *ip + blk > nswap)
		panic("vstodb *ip");
	dbp->db_size = MIN(vssize, blk - vsbase);
	dbp->db_base = *ip + (rev ? blk - (vsbase + dbp->db_size) : vsbase);
}

/*VARARGS1*/
panic(cp, a, b, c, d)
	char *cp;
{
	printf(cp, a, b, c, d);
	printf("\n");
	longjmp(cont_frame, 1);
}

/* 
 * Debugging routine to print out page table.
 */
#ifdef	DEBUG
showpt(pt, n, s)
	struct pte *pt;
	int n;
	char *s;
{
	register struct pte *p;
	register int i;

	printf("*** %s page table\n", s);
	for (i = 0, p = pt; i < n; i++, p++)
		if (! p->pg_fod)
			printf("%d: %x\n", i, p->pg_pfnum);
}
#endif
