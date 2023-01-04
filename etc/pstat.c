/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char *sccsid = "@(#)pstat.c 1.1 86/09/24 SMI"; /* from UCB 5.8 5/5/86 */
#endif not lint

/*
 * Print system stuff
 */

#define mask(x) (x&0377)
#define	clear(x) ((int)x-KERNELBASE)

#include <stdio.h>
#include <sys/param.h>
#include <sys/dir.h>
#define	KERNEL
#include <sys/file.h>
#undef	KERNEL
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/text.h>
#include <sys/vnode.h>
#include <ufs/inode.h>
#include <sys/map.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vm.h>
#include <nlist.h>
#include <machine/pte.h>

char	*fcore	= "/dev/kmem";
char	*fmem	= "/dev/mem";
char	*fnlist	= "/vmunix";
int	fc, fm;

union {
	struct	user user;
	char	upages[UPAGES][NBPG];
} user;

#define pcboffset	((char *)&user.user.u_pcb - (char *)&user.user)

struct nlist nl[] = {
#define	SINODE	0
	{ "_inode" },
#define	STEXT	1
	{ "_text" },
#define	SPROC	2
	{ "_proc" },
#define	SDZ	3
	{ "_dz_tty" },
#define	SNDZ	4
	{ "_dz_cnt" },
#define	SKL	5
	{ "_cons" },
#define	SFIL	6
	{ "_file" },
#define	USRPTMA	7
	{ "_Usrptmap" },
#define	USRPT	8
	{ "_usrpt" },
#define	SWAPMAP	9
	{ "_swapmap" },
#define	SDH	10
	{ "_dh11" },
#define	SNDH	11
	{ "_ndh11" },
#define	SNPROC	12
	{ "_nproc" },
#define	SNTEXT	13
	{ "_ntext" },
#define	SNFILE	14
	{ "_nfile" },
#define	SNINODE	15
	{ "_ninode" },
#define	SNSWAPMAP 16
	{ "_nswapmap" },
#define	SPTY	17
	{ "_pt_tty" },
#define	SDMMIN	18
	{ "_dmmin" },
#define	SDMMAX	19
	{ "_dmmax" },
#define	SNSWDEV	20
	{ "_nswdev" },
#define	SSWDEVT	21
	{ "_swdevt" },
#define	SDMF	22
	{ "_dmf_tty" },
#define	SNDMF	23
	{ "_ndmf" },
#define	SNPTY	24
	{ "_npty" },
#define	SDHU	25
	{ "_dhu_tty" },
#define	SNDHU	26
	{ "_ndhu" },
#define	SDMZ	27
	{ "_dmz_tty" },
#define	SNDMZ	28
	{ "_ndmz" },
#define	SZS	29
	{ "_zs_tty" },
#define	SNZS	30
	{ "_nzs" },
#define	SMTI	31
	{ "_mti_tty" },
#define	SNMTI	32
	{ "_nmti" },
#define	SYSMAP	33
	{ "_Sysmap" },
	{ "" }
};

int	inof;
int	txtf;
int	prcf;
int	ttyf;
int	usrf;
long	ubase;
int	filf;
int	swpf;
int	totflg;
char	partab[1];
struct	cdevsw	cdevsw[1];
struct	bdevsw	bdevsw[1];
int	allflg;
int	kflg;
struct	pte *Usrptma;
struct	pte *usrpt;
u_long	getw();
off_t	mkphys();

main(argc, argv)
char **argv;
{
	register char *argp;
	int allflags;

	argc--, argv++;
	while (argc > 0 && **argv == '-') {
		argp = *argv++;
		argp++;
		argc--;
		while (*argp++)
		switch (argp[-1]) {

		case 'T':
			totflg++;
			break;

		case 'a':
			allflg++;
			break;

		case 'i':
			inof++;
			break;

		case 'k':
			kflg++;
			fcore = fmem = "/vmcore";
			break;

		case 'x':
			txtf++;
			break;

		case 'p':
			prcf++;
			break;

		case 't':
			ttyf++;
			break;

		case 'u':
			if (argc == 0) {
				fprintf(stderr, "pstat: No address specified for -u\n");
				exit(1);
			}
			argc--;
			usrf++;
			sscanf(*argv++, "%x", &ubase);
			break;

		case 'f':
			filf++;
			break;
		case 's':
			swpf++;
			break;
		default:
			usage();
			exit(1);
		}
	}
	if (argc>1) {
		fcore = fmem = argv[1];
		kflg++;
	}
	if ((fc = open(fcore, 0)) < 0) {
		fprintf(stderr, "pstat: ");
		perror(fcore);
		exit(1);
	}
	if ((fm = open(fmem, 0)) < 0) {
		fprintf(stderr, "pstat: ");
		perror(fmem);
		exit(1);
	}
	if (argc>0)
		fnlist = argv[0];
	nlist(fnlist, nl);
	usrpt = (struct pte *)nl[USRPT].n_value;
	Usrptma = (struct pte *)nl[USRPTMA].n_value;
	if (nl[0].n_type == 0) {
		fprintf(stderr, "pstat: No namelist for %s\n", fnlist);
		exit(1);
	}
	allflags = filf | totflg | inof | prcf | txtf | ttyf | usrf | swpf;
	if (allflags == 0) {
		fprintf(stderr, "pstat: one or more of -[aixptfsu] is required\n");
		exit(1);
	}
	if (filf||totflg)
		dofile();
	if (inof||totflg)
		doinode();
	if (prcf||totflg)
		doproc();
	if (txtf||totflg)
		dotext();
	if (ttyf)
		dotty();
	if (usrf)
		dousr();
	if (swpf||totflg)
		doswap();
	exit(0);
}

usage()
{

	fprintf(stderr, "usage: pstat -[aixptfs] [-u ubase] [system] [core]\n");
}

doinode()
{
	register struct inode *ip;
	register struct vnode *vp;
	struct inode *xinode, *ainode;
	register int nin;
	int ninode;

	nin = 0;
	ninode = getw(nl[SNINODE].n_value);
	xinode = (struct inode *)calloc(ninode, sizeof (struct inode));
	ainode = (struct inode *)getw(nl[SINODE].n_value);
	if (ninode < 0 || ninode > 10000) {
		fprintf(stderr, "number of inodes is preposterous (%d)\n",
			ninode);
		return;
	}
	if (xinode == NULL) {
		fprintf(stderr, "can't allocate memory for inode table\n");
		return;
	}
	lseek(fc, mkphys((off_t)ainode), 0);
	read(fc, xinode, ninode * sizeof(struct inode));
	for (ip = xinode; ip < &xinode[ninode]; ip++)
		if (ip->i_vnode.v_count)
			nin++;
	if (totflg) {
		printf("%3d/%3d inodes\n", nin, ninode);
		return;
	}
	printf("%d/%d active inodes\n", nin, ninode);
printf("  ILOC   IFLAG   IDEVICE   INO   MODE NLK  UID  SIZE/DEV VFLAG CNT SHC EXC TYPE\n");
	if (kflg)
		ainode = (struct inode *)((int)ainode + KERNELBASE);
	for (ip = xinode; ip < &xinode[ninode]; ip++) {
		vp = &ip->i_vnode;
		if (vp->v_count == 0)
			continue;
		printf("%8.1x ", ainode + (ip - xinode));
		putf(ip->i_flag&IACC, 'A');
		putf(ip->i_flag&ICHG, 'C');
		putf(ip->i_flag&ILOCKED, 'L');
		putf(ip->i_flag&IREF, 'R');
		putf(ip->i_flag&IUPD, 'U');
		putf(ip->i_flag&IWANT, 'W');
		printf("%3d,%3d", major(ip->i_dev), minor(ip->i_dev));
		printf("%6d", ip->i_number);
		printf("%7o", ip->i_mode & 0xffff);
		printf("%4d", ip->i_nlink);
		printf("%5d", ip->i_uid);
		if ((ip->i_mode&IFMT)==IFBLK || (ip->i_mode&IFMT)==IFCHR)
			printf("%6d,%3d", major(ip->i_rdev), minor(ip->i_rdev));
		else
			printf("%10ld", ip->i_size);
		printf(" ");
		putf(vp->v_flag&VROOT, 'R');
		putf(vp->v_flag&VSHLOCK, 'S');
		putf(vp->v_flag&VEXLOCK, 'E');
		putf(vp->v_flag&VTEXT, 'T');
		putf(vp->v_flag&VLWAIT, 'Z');
		printf("%4d", vp->v_count&0377);
		printf("%4d", vp->v_shlockc&0377);
		printf("%4d", vp->v_exlockc&0377);
		switch (vp->v_type) {
			case VNON:
				printf(" VNON");
				break;
			case VREG:
				printf(" VREG");
				break;
			case VDIR:
				printf(" VDIR");
				break;
			case VBLK:
				printf(" VBLK");
				break;
			case VCHR:
				printf(" VCHR");
				break;
			case VLNK:
				printf(" VLNK");
				break;
			case VSOCK:
				printf(" VSOC");
				break;
			case VBAD:
				printf(" VBAD");
				break;
			case VFIFO:
				printf(" VFIFO");
				break;
			default:
				printf(" ????");
				break;
		}
		printf("\n");
	}
	free(xinode);
}

u_long
getw(loc)
	off_t loc;
{
	u_long word;

	if (kflg)
		loc -= KERNELBASE;
	lseek(fc, loc, 0);
	read(fc, &word, sizeof (word));
	return (word);
}

putf(v, n)
{

	if (v)
		printf("%c", n);
	else
		printf(" ");
}

dotext()
{
	register struct text *xp;
	int ntext;
	struct text *xtext, *atext;
	int ntx;

	ntx = 0;
	ntext = getw(nl[SNTEXT].n_value);
	xtext = (struct text *)calloc(ntext, sizeof (struct text));
	atext = (struct text *)getw(nl[STEXT].n_value);
	if (ntext < 0 || ntext > 10000) {
		fprintf(stderr, "number of texts is preposterous (%d)\n",
			ntext);
		return;
	}
	if (xtext == NULL) {
		fprintf(stderr, "can't allocate memory for text table\n");
		return;
	}
	lseek(fc, mkphys((off_t)atext), 0);
	read(fc, xtext, ntext * sizeof (struct text));
	for (xp = xtext; xp < &xtext[ntext]; xp++)
		if (xp->x_vptr!=NULL)
			ntx++;
	if (totflg) {
		printf("%3d/%3d texts\n", ntx, ntext);
		return;
	}
	printf("%d/%d active texts\n", ntx, ntext);
	printf("\
   LOC   FLAGS DADDR     CADDR  RSS SIZE      VPTR  CNT CCNT\n");
	if (kflg)
		atext = (struct text *)((int)atext + KERNELBASE);
	for (xp = xtext; xp < &xtext[ntext]; xp++) {
		if (xp->x_vptr == NULL)
			continue;
		printf("%8.1x", atext + (xp - xtext));
		printf(" ");
		putf(xp->x_flag&XPAGV, 'P');
		putf(xp->x_flag&XTRC, 'T');
		putf(xp->x_flag&XWRIT, 'W');
		putf(xp->x_flag&XLOAD, 'L');
		putf(xp->x_flag&XLOCK, 'K');
		putf(xp->x_flag&XWANT, 'w');
		printf("%5x", xp->x_daddr[0]);
		printf("%10x", xp->x_caddr);
		printf("%5d", xp->x_rssize);
		printf("%5d", xp->x_size);
		printf("%10.1x", xp->x_vptr);
		printf("%5d", xp->x_count&0377);
		printf("%5d", xp->x_ccount);
		printf("\n");
	}
	free(xtext);
}

doproc()
{
	struct proc *xproc, *aproc;
	int nproc;
	register struct proc *pp;
	register loc, np;
	struct pte apte;
	struct pte uutl[UPAGES];

	nproc = getw(nl[SNPROC].n_value);
	xproc = (struct proc *)calloc(nproc, sizeof (struct proc));
	aproc = (struct proc *)getw(nl[SPROC].n_value);
	if (nproc < 0 || nproc > 10000) {
		fprintf(stderr, "number of procs is preposterous (%d)\n",
			nproc);
		return;
	}
	if (xproc == NULL) {
		fprintf(stderr, "can't allocate memory for proc table\n");
		return;
	}
	lseek(fc, mkphys((off_t)aproc), 0);
	read(fc, xproc, nproc * sizeof (struct proc));
	np = 0;
	for (pp=xproc; pp < &xproc[nproc]; pp++)
		if (pp->p_stat)
			np++;
	if (totflg) {
		printf("%3d/%3d processes\n", np, nproc);
		return;
	}
	printf("%d/%d processes\n", np, nproc);
	printf("   LOC    S    F POIP PRI      SIG  UID SLP TIM  CPU  NI   PGRP    PID   PPID    ADDR   RSS SRSS SIZE    WCHAN    LINK   TEXTP\n");
	if (kflg)
		aproc = (struct proc *)((int)aproc + KERNELBASE);
	for (pp=xproc; pp<&xproc[nproc]; pp++) {
		if (pp->p_stat==0 && allflg==0)
			continue;
		printf("%8x", aproc + (pp - xproc));
		printf(" %2d", pp->p_stat);
		printf(" %4x", pp->p_flag & 0xffff);
		printf(" %4d", pp->p_poip);
		printf(" %3d", pp->p_pri);
		printf(" %8x", pp->p_sig);
		printf(" %4d", pp->p_uid);
		printf(" %3d", pp->p_slptime);
		printf(" %3d", pp->p_time);
		printf(" %4d", pp->p_cpu&0377);
		printf(" %3d", pp->p_nice);
		printf(" %6d", pp->p_pgrp);
		printf(" %6d", pp->p_pid);
		printf(" %6d", pp->p_ppid);
		if (pp->p_flag & SLOAD) {
			if (getkpte(pp->p_addr, UPAGES, uutl) == 1)
				printf(" %8x",uutl[pcboffset/CLBYTES].pg_pfnum);
			else
				printf(" ????????");
		} else
			printf(" %8x", pp->p_swaddr);
		printf(" %4x", pp->p_rssize);
		printf(" %4x", pp->p_swrss);
		printf(" %5x", pp->p_dsize+pp->p_ssize);
		printf(" %7x", (int)pp->p_wchan & 0x0fffffff);
		printf(" %7x", (int)pp->p_link & 0x0fffffff);
		printf(" %7x", (int)pp->p_textp & 0x0fffffff);
		printf("\n");
	}
	free(xproc);
}

static char mesg[] =
" # RAW CAN OUT     MODE     ADDR DEL COL     STATE  PGRP DISC\n";
static int ttyspace = 128;
static struct tty *tty;

dotty()
{
	extern char *malloc();

	if ((tty = (struct tty *)malloc(ttyspace * sizeof(*tty))) == 0) {
		perror("pstat");
		return;
	}
	printf("1 cons\n");
	if (kflg)
		nl[SKL].n_value = clear(nl[SKL].n_value);
	lseek(fc, mkphys((off_t)nl[SKL].n_value), 0);
	read(fc, tty, sizeof(*tty));
	printf(mesg);
	ttyprt(&tty[0], 0);
	if (nl[SNDZ].n_type != 0)
		dottytype("dz", SDZ, SNDZ);
	if (nl[SNDH].n_type != 0)
		dottytype("dh", SDH, SNDH);
	if (nl[SNDMF].n_type != 0)
		dottytype("dmf", SDMF, SNDMF);
	if (nl[SNDHU].n_type != 0)
		dottytype("dhu", SDHU, SNDHU);
	if (nl[SNDMZ].n_type != 0)
		dottytype("dmz", SDMZ, SNDMZ);
	if (nl[SNZS].n_type != 0)
		dottytype("zs", SZS, SNZS);
	if (nl[SNMTI].n_type != 0)
		dottytype("mti", SMTI, SNMTI);
	if (nl[SNPTY].n_type != 0)
		dottytype("pty", SPTY, SNPTY);
}

dottytype(name, type, number)
char *name;
{
	int ntty;
	register struct tty *tp;
	extern char *realloc();

	if (tty == (struct tty *)0)
		return;
	if (kflg) {
		nl[number].n_value = clear(nl[number].n_value);
		nl[type].n_value = clear(nl[type].n_value);
	}
	lseek(fc, mkphys((off_t)nl[number].n_value), 0);
	read(fc, &ntty, sizeof(ntty));
	printf("%d %s lines\n", ntty, name);
	if (ntty > ttyspace) {
		ttyspace = ntty;
		if ((tty = (struct tty *)realloc(tty, ttyspace * sizeof(*tty))) == 0) {
			perror("pstat");
			return;
		}
	}
	lseek(fc, mkphys((off_t)nl[type].n_value), 0);
	read(fc, tty, ntty * sizeof(struct tty));
	printf(mesg);
	for (tp = tty; tp < &tty[ntty]; tp++)
		ttyprt(tp, tp - tty);
}

ttyprt(atp, line)
struct tty *atp;
{
	register struct tty *tp;

	printf("%2d", line);
	tp = atp;
	switch (tp->t_line) {

/*
	case NETLDISC:
		if (tp->t_rec)
			printf("%4d%4d", 0, tp->t_inbuf);
		else
			printf("%4d%4d", tp->t_inbuf, 0);
		break;
*/

	default:
		printf("%4d%4d", tp->t_rawq.c_cc, tp->t_canq.c_cc);
	}
	printf("%4d %8x %8x%4d%4d", tp->t_outq.c_cc, tp->t_flags,
		tp->t_addr, tp->t_delct, tp->t_col);
	putf(tp->t_state&TS_TIMEOUT, 'T');
	putf(tp->t_state&TS_WOPEN, 'W');
	putf(tp->t_state&TS_ISOPEN, 'O');
	putf(tp->t_state&TS_FLUSH, 'F');
	putf(tp->t_state&TS_CARR_ON, 'C');
	putf(tp->t_state&TS_BUSY, 'B');
	putf(tp->t_state&TS_ASLEEP, 'A');
	putf(tp->t_state&TS_XCLUDE, 'X');
	putf(tp->t_state&TS_TTSTOP, 'S');
	putf(tp->t_state&TS_HUPCLS, 'H');
	printf("%6d", tp->t_pgrp);
	switch (tp->t_line) {

	case OTTYDISC:
		printf("\n");
		break;

	case NTTYDISC:
		printf(" ntty\n");
		break;

	case NETLDISC:
		printf(" berknet\n");
		break;

	case TABLDISC:
		printf(" tab\n");
		break;

	case NTABLDISC:
		printf(" ntab\n");
		break;

	case MOUSELDISC:
		printf(" mouse\n");
		break;

	case KBDLDISC:
		printf(" keyboard\n");
		break;

	default:
		printf(" %d\n", tp->t_line);
	}
}

/*
 * ubase is the physical page number which contains the pcb of the
 * u area.  It is assumed than pcb is on a page boundary if the user area
 * is larger than CLBYTES.
 */
dousr()
{
#define	U user.user
	struct ucred UC;
	register i, j, *ip;

	lseek(fm, ubase * NBPG, 0);
	if (sizeof(U) > CLBYTES)
		read(fm, &U.u_pcb, sizeof(U) - pcboffset);
	else
		read(fm, &U, sizeof(U));
	if (kflg)
		clear(U.u_cred);
	lseek(fc, mkphys((off_t)U.u_cred), 0);
	read(fc, &UC, sizeof(UC));
	printf("pcb");
	ip = (int *)&U.u_pcb;
	while (ip < &U.u_arg[0]) {
		if ((ip - (int *)&U.u_pcb) % 4 == 0)
			printf("\t");
		printf("%x ", *ip++);
		if ((ip - (int *)&U.u_pcb) % 4 == 0)
			printf("\n");
	}
	if ((ip - (int *)&U.u_pcb) % 4 != 0)
		printf("\n");
	printf("arg\t");
	for (i=0; i<5; i++)
		printf(" %.1x", U.u_arg[i]);
	printf("\n");
	printf("ssave");
	for (i=0; i<sizeof(label_t)/sizeof(int); i++) {
		if (i%5==0)
			printf("\t");
		printf("%9.1x", U.u_ssave.val[i]);
		if (i%5==4)
			printf("\n");
	}
	if (i%5)
		printf("\n");
	printf("error %d\n", U.u_error);
	printf("uids\t%d,%d,%d,%d\n", UC.cr_uid,UC.cr_gid,UC.cr_ruid,
	    UC.cr_rgid);
	printf("procp\t%.1x\n", U.u_procp);
	printf("ap\t%.1x\n", U.u_ap);
	printf("r_val?\t%.1x %.1x\n", U.u_r.r_val1, U.u_r.r_val2);
	printf("cdir rdir %.1x %.1x\n", U.u_cdir, U.u_rdir);
	printf("file");
	for (i=0; i<NOFILE; i++) {
		if (i%5==0)
			printf("\t");
		printf("%9.1x", U.u_ofile[i]);
		if (i%5==4)
			printf("\n");
	}
	printf("\n");
	printf("pofile");
	for (i=0; i<NOFILE; i++) {
		if (i%5==0)
			printf("\t");
		printf("%9.1x", U.u_pofile[i]);
		if (i%5==4)
			printf("\n");
	}
	printf("\n");
	printf("ssave");
	for (i=0; i<sizeof(label_t)/sizeof(int); i++) {
		if (i%5==0)
			printf("\t");
		printf("%9.1x", U.u_ssave.val[i]);
		if (i%5==4)
			printf("\n");
	}
	if (i%5)
		printf("\n");
	printf("sigs");
	for (i=0; i<NSIG; i++) {
		if (i%5==0)
			printf("\t");
		printf("%9.1x ", U.u_signal[i]);
		if (i%5==4)
			printf("\n");
	}
	printf("\n");
	printf("code\t%.1x\n", U.u_code);
	printf("ar0\t%.1x\n", U.u_ar0);
	printf("prof\t%X %X %X %X\n", U.u_prof.pr_base, U.u_prof.pr_size,
	    U.u_prof.pr_off, U.u_prof.pr_scale);
	printf("\neosys\t%d\n", U.u_eosys);
	printf("ttyp\t%.1x\n", U.u_ttyp);
	printf("ttyd\t%d,%d\n", major(U.u_ttyd), minor(U.u_ttyd));
	printf("exdata\t");
	ip = (int *)&U.u_exdata;
	for (i = 0; i < 8; i++)
		printf("%.1D ", *ip++);
	printf("\n");
	printf("comm\t%.14s\n", U.u_comm);
	printf("start\t%D\n", U.u_start);
	printf("acflag\t%D\n", U.u_acflag);
	printf("cmask\t%D\n", U.u_cmask);
	printf("sizes\t%.1x %.1x %.1x\n", U.u_tsize, U.u_dsize, U.u_ssize);
	printf("ru\t");
	ip = (int *)&U.u_ru;
	for (i = 0; i < sizeof(U.u_ru)/sizeof(int); i++)
		printf("%D ", ip[i]);
	printf("\n");
	ip = (int *)&U.u_cru;
	printf("cru\t");
	for (i = 0; i < sizeof(U.u_cru)/sizeof(int); i++)
		printf("%D ", ip[i]);
	printf("\n");
}

oatoi(s)
char *s;
{
	register v;

	v = 0;
	while (*s)
		v = (v<<3) + *s++ - '0';
	return(v);
}

dofile()
{
	int nfile;
	struct file *xfile, *afile;
	register struct file *fp;
	register nf;
	int loc;
	static char *dtypes[] = { "???", "inode", "socket" };

	nf = 0;
	nfile = getw(nl[SNFILE].n_value);
	xfile = (struct file *)calloc(nfile, sizeof (struct file));
	afile = (struct file *)getw(nl[SFIL].n_value);
	if (nfile < 0 || nfile > 10000) {
		fprintf(stderr, "number of files is preposterous (%d)\n",
			nfile);
		return;
	}
	if (xfile == NULL) {
		fprintf(stderr, "can't allocate memory for file table\n");
		return;
	}
	lseek(fc, mkphys((off_t)afile), 0);
	read(fc, xfile, nfile * sizeof (struct file));
	for (fp=xfile; fp < &xfile[nfile]; fp++)
		if (fp->f_count)
			nf++;
	if (totflg) {
		printf("%3d/%3d files\n", nf, nfile);
		return;
	}
	printf("%d/%d open files\n", nf, nfile);
	printf("   LOC   TYPE    FLG     CNT  MSG    DATA    OFFSET\n");
	if (kflg)
		afile = (struct file *)((int)afile + KERNELBASE);
	for (fp=xfile,loc=(int)afile; fp < &xfile[nfile]; fp++,loc+=sizeof(xfile[0])) {
		if (fp->f_count==0)
			continue;
		printf("%8x ", loc);
		if (fp->f_type <= DTYPE_SOCKET)
			printf("%-8.8s", dtypes[fp->f_type]);
		else
			printf("8d", fp->f_type);
		putf(fp->f_flag&FREAD, 'R');
		putf(fp->f_flag&FWRITE, 'W');
		putf(fp->f_flag&FAPPEND, 'A');
		putf(fp->f_flag&FSHLOCK, 'S');
		putf(fp->f_flag&FEXLOCK, 'X');
		putf(fp->f_flag&FASYNC, 'I');
		printf("  %3d", mask(fp->f_count));
		printf("  %3d", mask(fp->f_msgcount));
		printf("  %8.1x", fp->f_data);
		if (fp->f_offset < 0)
			printf("  %x\n", fp->f_offset);
		else
			printf("  %ld\n", fp->f_offset);
	}
	free(xfile);
}

#ifdef sun
int	swap = -1;
char	*swapf = "/dev/drum";

getu(p)
	struct proc *p;
{
	struct pte uutl[UPAGES];
	register int i;
	int ncl, size;

	if (!kflg && swap < 0) {
		swap = open(swapf, 0);
		if (swap < 0) {
			perror(swapf);
			exit(1);
		}
	}
	size = roundup(sizeof (struct user), DEV_BSIZE);
	if ((p->p_flag & SLOAD) == 0) {
		if (swap < 0)
			return (0);
		(void) lseek(swap, (long)dtob(p->p_swaddr), 0);
		if (read(swap, (char *)&user.user, size) != size) {
			printf("pstat: can't read u for pid %d from %s\n",
			    p->p_pid, swapf);
			return (0);
		}
		return (1);
	}
	if (getkpte(p->p_addr, UPAGES, uutl) == 0)
		return (0);
	ncl = (size + CLBYTES - 1) / CLBYTES;
	while (--ncl >= 0) {
		i = ncl * CLSIZE;
		lseek(fm, (long)ctob(uutl[i].pg_pfnum), 0);
		if (read(fm, user.upages[i], CLBYTES) != CLBYTES) {
			printf("pstat: can't read page %d of u of pid %d from %s\n",
			    uutl[i].pg_pfnum, p->p_pid, fmem);
			return (0);
		}
	}
	return (1);
}
#endif sun

int dmmin, dmmax, nswdev;

doswap()
{
	struct proc *proc;
	int nproc;
	struct text *xtext;
	int ntext;
	struct map *swapmap;
	int nswapmap;
	struct swdevt *swdevt, *sw;
	register struct proc *pp;
	int nswap, used, tused, free, waste;
	int db, sb;
	register struct mapent *me;
	register struct text *xp;
	int i, j, block;
	int dmap[NDMAP];

	nproc = getw(nl[SNPROC].n_value);
	ntext = getw(nl[SNTEXT].n_value);
	if (nproc < 0 || nproc > 10000 || ntext < 0 || ntext > 10000) {
		fprintf(stderr, "number of procs/texts is preposterous (%d, %d)\n",
			nproc, ntext);
		return;
	}
	proc = (struct proc *)calloc(nproc, sizeof (struct proc));
	if (proc == NULL) {
		fprintf(stderr, "can't allocate memory for proc table\n");
		exit(1);
	}
	xtext = (struct text *)calloc(ntext, sizeof (struct text));
	if (xtext == NULL) {
		fprintf(stderr, "can't allocate memory for text table\n");
		exit(1);
	}
	nswapmap = getw(nl[SNSWAPMAP].n_value);
	swapmap = (struct map *)calloc(nswapmap, sizeof (struct map));
	if (swapmap == NULL) {
		fprintf(stderr, "can't allocate memory for swapmap\n");
		exit(1);
	}
	nswdev = getw(nl[SNSWDEV].n_value);
	swdevt = (struct swdevt *)calloc(nswdev, sizeof (struct swdevt));
	if (swdevt == NULL) {
		fprintf(stderr, "can't allocate memory for swdevt table\n");
		exit(1);
	}
	lseek(fc, mkphys((off_t)nl[SSWDEVT].n_value), L_SET);
	read(fc, swdevt, nswdev * sizeof (struct swdevt));
	lseek(fc, mkphys((off_t)getw(nl[SPROC].n_value)), 0);
	read(fc, proc, nproc * sizeof (struct proc));
	lseek(fc, mkphys((off_t)getw(nl[STEXT].n_value)), 0);
	read(fc, xtext, ntext * sizeof (struct text));
	lseek(fc, mkphys((off_t)getw(nl[SWAPMAP].n_value)), 0);
	read(fc, swapmap, nswapmap * sizeof (struct map));
	mapfree(swapmap) = nswapmap - 3;
	mapname(swapmap+nswapmap-1) = "swap";
	dmmin = getw(nl[SDMMIN].n_value);
	dmmax = getw(nl[SDMMAX].n_value);
	nswap = 0;
	for (sw = swdevt; sw < &swdevt[nswdev]; sw++)
		if (sw->sw_freed)
			nswap += sw->sw_nblks;
	free = 0;
	for (me = (struct mapent *)(swapmap+1);
	    me < (struct mapent *)&swapmap[nswapmap]; me++)
		free += me->m_size;
	tused = 0;
	for (xp = xtext; xp < &xtext[ntext]; xp++)
		if (xp->x_vptr!=NULL) {
			tused += ctod(clrnd(xp->x_size));
			if (xp->x_flag & XPAGV)
				tused += ctod(clrnd(ctopt(xp->x_size)));
		}
	used = tused;
	waste = 0;
	for (pp = proc; pp < &proc[nproc]; pp++) {
		if (pp->p_stat == 0 || pp->p_stat == SZOMB)
			continue;
		if (pp->p_flag & SSYS)
			continue;
		db = ctod(pp->p_dsize), sb = up(db);
		used += sb;
		waste += sb - db;
		db = ctod(pp->p_ssize), sb = up(db);
		used += sb;
		waste += sb - db;
		if ((pp->p_flag&SLOAD) == 0)
			used += ctod(vusize(pp));
#ifdef sun
		if (getu(pp) && user.user.u_hole.uh_last) {
			used -= ctod(user.user.u_hole.uh_last -
			    user.user.u_hole.uh_first + 1);
		}
#endif sun
	}
	if (totflg) {
#define	btok(x)	((x) / (1024 / DEV_BSIZE))
		printf("%3d/%3d 00k swap\n",
		    btok(used/100), btok((used+free)/100));
		return;
	}
	printf("%dk used (%dk text), %dk free, %dk wasted, %dk missing\n",
	    btok(used), btok(tused), btok(free), btok(waste),
/* a dmmax/2 block goes to argmap */
	    btok(nswap - dmmax/2 - (used + free)));
	block = dmmin;
	for (i=0, j=0; i<NDMAP; i++) {
		dmap[i] = rmalloc(swapmap, block);
		if (dmap[i] == 0)
			break;
		j += block;
		if (block < dmmax)
			block *= 2;
	}
	printf("max process allocable = %dk\n", btok(j));
	block = dmmin;
	for (j=0; j<i; j++) {
		rmfree(swapmap, block, dmap[j]);
		if (block < dmmax)
			block *= 2;
	}
	printf("avail: ");
	for (i = dmmax; i >= dmmin; i /= 2) {
		j = 0;
		while (rmalloc(swapmap, i) != 0)
			j++;
		if (j)
			printf("%d*%dk ", j, btok(i));
	}
	free = 0;
	for (me = (struct mapent *)(swapmap+1);
	    me < (struct mapent *)&swapmap[nswapmap]; me++)
		free += me->m_size;
	printf("%d*1k\n", btok(free));
}

up(size)
	register int size;
{
	register int i, block;

	i = 0;
	block = dmmin;
	while (i < size) {
		i += block;
		if (block < dmmax)
			block *= 2;
	}
	return (i);
}

/*
 * Compute number of pages to be allocated to the u. area
 * and data and stack area page tables, which are stored on the
 * disk immediately after the u. area.
 */
vusize(p)
	register struct proc *p;
{
	register int tsz = p->p_tsize / NPTEPG;

	/*
	 * We do not need page table space on the disk for page
	 * table pages wholly containing text. 
	 */
	return (clrnd(UPAGES +
	    clrnd(ctopt(p->p_tsize+p->p_dsize+p->p_ssize+UPAGES)) - tsz));
}

/*
 * Allocate 'size' units from the given
 * map. Return the base of the allocated space.
 * In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 *
 * Algorithm is first-fit.
 *
 * This routine knows about the interleaving of the swapmap
 * and handles that.
 */
long
rmalloc(mp, size)
	register struct map *mp;
	long size;
{
	register struct mapent *ep = mapstart(mp);
	register long addr;
	register struct mapent *bp;
	swblk_t first, rest;

	if (size <= 0 || size > dmmax)
		return (0);
	/*
	 * Search for a piece of the resource map which has enough
	 * free space to accomodate the request.
	 */
	for (bp = ep; bp->m_size; bp++) {
		if (bp->m_size >= size) {
			/*
			 * If allocating from swapmap,
			 * then have to respect interleaving
			 * boundaries.
			 */
			if (nswdev > 1 &&
			    (first = dmmax - bp->m_addr%dmmax) < bp->m_size) {
				if (bp->m_size - first < size)
					continue;
				addr = bp->m_addr + first;
				rest = bp->m_size - first - size;
				bp->m_size = first;
				if (rest)
					rmfree(mp, rest,(long)(addr+size));
				return (addr);
			}
			/*
			 * Allocate from the map.
			 * If there is no space left of the piece
			 * we allocated from, move the rest of
			 * the pieces to the left and increment the free
			 * segment count.
			 */
			addr = bp->m_addr;
			bp->m_addr += size;
			if ((bp->m_size -= size) == 0) {
				do {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
				} while ((bp-1)->m_size = bp->m_size);
				mapfree(mp)++;
			}
			if (addr % CLSIZE)
				return (0);
			return (addr);
		}
	}
	return (0);
}

/*
 * Free the previously allocated space at addr
 * of size units into the specified map.
 * Sort addr into map and combine on
 * one or both ends if possible.
 */
rmfree(mp, size, addr)
	struct map *mp;
	long size, addr;
{
	struct mapent *firstbp;
	register struct mapent *bp;
	register int t;

	/*
	 * Both address and size must be
	 * positive, or the protocol has broken down.
	 */
	if (addr <= 0 || size <= 0)
		goto badrmfree;
	/*
	 * Locate the piece of the map which starts after the
	 * returned space (or the end of the map).
	 */
retry:
	firstbp = bp = mapstart(mp);
	for (; bp->m_addr <= addr && bp->m_size != 0; bp++)
		continue;
	/*
	 * If the piece on the left abuts us,
	 * then we should combine with it.
	 */
	if (bp > firstbp && (bp-1)->m_addr+(bp-1)->m_size >= addr) {
		/*
		 * Check no overlap (internal error).
		 */
		if ((bp-1)->m_addr+(bp-1)->m_size > addr)
			goto badrmfree;
		/*
		 * Add into piece on the left by increasing its size.
		 */
		(bp-1)->m_size += size;
		/*
		 * If the combined piece abuts the piece on
		 * the right now, compress it in also,
		 * by shifting the remaining pieces of the map over.
		 * Also, increment free segment count.
		 */
		if (bp->m_size && addr+size >= bp->m_addr) {
			if (addr+size > bp->m_addr)
				goto badrmfree;
			(bp-1)->m_size += bp->m_size;
			while (bp->m_size) {
				bp++;
				(bp-1)->m_addr = bp->m_addr;
				(bp-1)->m_size = bp->m_size;
			}
			mapfree(mp)++;
		}
		goto done;
	}
	/*
	 * Don't abut on the left, check for abutting on
	 * the right.
	 */
	if (addr+size >= bp->m_addr && bp->m_size) {
		if (addr+size > bp->m_addr)
			goto badrmfree;
		bp->m_addr -= size;
		bp->m_size += size;
		goto done;
	}
	/*
	 * Don't abut at all.  Check for map overflow.
	 * Discard the smaller of the last/next-to-last entries.
	 * Then retry the rmfree operation.
	 */
	if (mapfree(mp) == 0) {
		/* locate final entry */
		for (firstbp = bp; firstbp->m_size != 0; firstbp++)
			continue;

		/* point to smaller of the last two segments */
		bp = firstbp - 1;
		if (bp->m_size > (bp-1)->m_size)
			bp--;
		printf("%s: rmap ovflo, lost [%d,%d)\n", mapname(firstbp),
		    (bp)->m_addr, (bp)->m_addr+(bp)->m_size);

		/* destroy one entry, compressing down; inc free count */
		bp[0] = bp[1];
		bp[1].m_size = 0;
		mapfree(mp)++;
		goto retry;
	}
	/*
	 * Make a new entry and push the remaining ones up
	 */
	do {
		t = bp->m_addr;
		bp->m_addr = addr;
		addr = t;
		t = bp->m_size;
		bp->m_size = size;
		bp++;
	} while (size = t);
	mapfree(mp)--;		/* one less free segment remaining */
done:
	return;
badrmfree:
	printf("bad rmfree\n");
}

/*
 * "addr" is a kern virt addr and does not correspond
 * to a phys addr after subtracting off KERNELBASE
 * since it might have been valloc'd in the kernel.
 *
 * We return the phys addr by simulating kernel vm (/dev/kmem)
 * when we are reading a crash dump.
 */
off_t
mkphys(addr)
	off_t addr;
{
	register off_t o;

	if (!kflg)
		return (addr);
	o = addr & PGOFSET;
	if (addr >= KERNELBASE)
		addr -= KERNELBASE;
	addr >>= PGSHIFT;
	addr &= PG_PFNUM;
	addr *= NBPW;
	lseek(fc, clear(nl[SYSMAP].n_value + addr), 0);
	read(fc, &addr, sizeof (addr));
	addr = ((addr & PG_PFNUM) << PGSHIFT) | o;
	return (addr);
}

/*
 * Get npte ptes from kernel address ptep into array kpte.
 * XXX - ptes must not cross a kernel page boundary.
 */
getkpte(ptep, npte, kpte)
	struct pte *ptep;
	int npte;
	struct pte kpte[];
{
	struct pte *pteaddr, apte;

	pteaddr = &Usrptma[btokmx(ptep)];
	if (kflg)
		pteaddr = (struct pte *)clear(pteaddr);
	lseek(fc, (long)pteaddr, 0);
	if (read(fc, (char *)&apte, sizeof (apte)) != sizeof (apte))
		return (0);
	lseek(fm, (long)ctob(apte.pg_pfnum) + (((int)ptep)&PGOFSET), 0);
	if (read(fm, (char *)kpte, npte * sizeof (struct pte)) !=
	    npte * sizeof (struct pte))
		return (0);
	return (1);
}
