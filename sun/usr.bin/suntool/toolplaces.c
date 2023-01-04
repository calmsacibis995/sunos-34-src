#ifndef lint
static
char sccsid[] = "@(#)toolplaces.c 1.3 87/01/07";
#endif
/* 
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/* 
 * 	TOOLPLACES
 *
 *	toolplaces.c,  Fri Jul  5 12:30:45 1985
 *	Print out positions of all tools on the desktop.
 *
 *	Craig Taylor, 
 *	Sun Microsystems
 *
 *	Options:
 *	  -u = update options that were specified.
 *	  -ua = update options and add option not specified.
 *	  -c = original command line without updated options.
 *	  -o = use old suntools format (should we stop supporting this?)
 *	  -O = old toolplaces format.  Does not open any of the system files.
 *
 *	This is a completely rewritten version of toolplaces.
 *	Ps(1) code was hacked over to provide the command line.
 *
 */

/* Used to walk thru /dev/kmem, etc. */
#include <sys/param.h>
#include <sys/user.h>
#include <stdio.h>
#include <ctype.h>
#include <nlist.h>
#include <pwd.h>
#include <sys/dir.h>
#include <sys/proc.h>
#include <machine/pte.h>
#include <sys/vm.h>
#include <sys/text.h>
#include <sys/stat.h>
#include <sys/mbuf.h>
#include <math.h>
/* Used to enumerate the tool positions */
#include <sys/file.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/cms.h>
#include <sunwindow/cms_mono.h>
#include <sunwindow/win_struct.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_screen.h>
#include <sunwindow/win_ioctl.h>
#include <suntool/wmgr.h>

extern char	 *strcpy(), *strcat(), *strncat();
extern long 	 lseek();

static int	  dmmin, dmmax;
static char	**get_argv(), **getcmd(),
		*sbrk();
static union {
    struct user user;
    char upages[UPAGES][NBPG];
} user;

#define u user.user

static struct pte *Usrptma, *usrpt;
static int	   nproc;
static off_t	   procp;

static char	  *kmemf, *memf, *swapf, *nlistf;
static int	   kmem, mem, swap = -1, argaddr;

static struct nlist nl[] = {
    {   "_proc" },
#define X_PROC		0
    {   "_Usrptmap" },
#define X_USRPTMA	1
    {   "_usrpt" },
#define X_USRPT		2
    {   "_text" },
#define X_TEXT		3
    {   "_nswap" },
#define X_NSWAP		4
    {   "_ccpu" },
#define X_CCPU		5
    {   "_nproc" },
#define X_NPROC		6
    {   "_ntext" },
#define X_NTEXT		7
    {   "_dmmin" },
#define X_DMMIN		8
    {   "_dmmax" },
#define X_DMMAX		9
    {   "" },
};

#define error(perror_p, emsg) { \
    (void)fprintf(stderr, "%s: %s\n", prog_name, emsg); \
    if ((int)perror_p) perror(prog_name); \
    }

#define error1(perror_p, emsg, earg) { \
    (void)fprintf(stderr, "%s: ", prog_name); \
    (void)fprintf(stderr, emsg, earg); \
    (void)fputc('\n', stderr); \
    if ((int)perror_p) perror(prog_name); \
    }

static enum {STD, UPDATE, UPDATE_ALL, OLDER, OLDEST, CMD_LINE} style;

static char *prog_name;
static struct colormapseg cms;
struct cms_map cmap;
static int fg_color, bg_color;
static unsigned char cmap_red[256];
static unsigned char cmap_green[256];
static unsigned char cmap_blue[256];

#ifdef STANDALONE
main(argc, argv)
#else
toolplaces_main(argc, argv)
#endif
int argc;
char **argv;
{   
    char **targv, name[WIN_NAMESIZE];
    int toolfd, wfd, link, iconic, i;
    struct rect rect, rectsaved, recttemp;
    struct screen screen;

    prog_name = *argv;
    parse_options(argc, argv);
    /*
     * Determine parent and get root fd
     */
    if (we_getparentwindow(name)) {
#ifndef lint
	error(0, "Parent window not in the environment.");
	error(0, "This program must be run under suntools.")
#endif
	exit(1);
    }
    if ((wfd = open(name, O_RDONLY, 0)) < 0) {
#ifndef lint
	error(1, "Parent would not open.");
#endif
    }
    (void)win_screenget(wfd, &screen);
    (void)close(wfd);
    if ((wfd = open(screen.scr_rootname, O_RDONLY, 0)) < 0)  {
#ifndef lint
	error1(1, "Screen(%s) would not open.", screen.scr_rootname);
#endif
    }
    /*
     * Printout rects of root children.
     */
    init_process_info();
    for (link = win_getlink(wfd, WL_OLDESTCHILD); link != WIN_NULLLINK;) {
	/* 
	 * Open tool window
	 */
	(void)win_numbertoname(link, name);
	if ((toolfd = open(name, O_RDONLY, 0)) < 0)   {
#ifndef lint
	    error(1, "One of the top level windows would not open.");
#endif
	}
	/* 
	 * Get rect data
	 */
	(void)win_getrect(toolfd, &rect);
	(void)win_getsavedrect(toolfd, &rectsaved);
	iconic = (win_getuserflags(toolfd)&WMGR_ICONIC)? 1: 0;
	if (iconic) {
	    recttemp = rect;
	    rect = rectsaved;
	    rectsaved = recttemp;
	}
	/* 
	 * Get original argv from the process owning the tool.
	 */
	if (style != OLDEST) targv = get_argv(win_getowner(toolfd));
	/* 
	 * Print entry in the style requested.
	 */
	switch (style) {
	  case STD:
	    (void)printf("%-10s -Wp %4d %3d -Ws %3d %3d -WP %4d %3d ",
		   targv[0], 
		   rect.r_left, rect.r_top, rect.r_width, rect.r_height,
		   rectsaved.r_left, rectsaved.r_top,
		   rectsaved.r_width, rectsaved.r_height);
	    if (iconic) (void)printf("-Wi ");
	    print_colormap(toolfd, &screen);
	    print_striped_args(targv+1);
	    break;

	  case UPDATE:
	  case UPDATE_ALL:
	    (void)printf("%-10s ", targv[0]);
	    get_colormap_entries(toolfd, &screen);
	    print_updated_args(targv+1, &rect, &rectsaved, iconic,
			       style == UPDATE_ALL);
	    break;
	    
	  case OLDER:
	    (void)printf("%-10s %4d %3d %3d %3d %4d %3d %3d %3d  %d  ",
		   targv[0], 
		   rect.r_left, rect.r_top, rect.r_width, rect.r_height,
		   rectsaved.r_left, rectsaved.r_top,
		   rectsaved.r_width, rectsaved.r_height,
		   iconic);
	    print_colormap(toolfd, &screen);
	    print_striped_args(targv+1);
	    break;

	  case OLDEST:  /* Useless format */
	    (void)printf("toolname    %4d %3d %3d %3d    %4d %3d %3d %3d     %d\n",
		   rect.r_left, rect.r_top, rect.r_width, rect.r_height,
		   rectsaved.r_left, rectsaved.r_top,
		   rectsaved.r_width, rectsaved.r_height,
		   iconic);
	    break;
	    
	  case CMD_LINE:
	    i = 0;
	    while (targv[i] != NULL) print_arg(targv[i++]);
	    (void)printf("\n");
	    break;

	}
	link = win_getlink(toolfd, WL_YOUNGERSIB); /* Next */
	(void)close(toolfd);
    }
    (void)close(wfd);
} /* main */

static
parse_options(argc, argv)
	int argc;
	char **argv;
{   
    style = STD;
    if (argc > 2)
    {   
	(void)fprintf(stderr, "Usage: %s [ -uoO ] [ -help ]\n", *argv);
	exit(1);
    }
    argv++;
    if (argc == 2) {
	if (! strcmp("-a", *argv)) style = STD;
	else if (! strcmp("-u", *argv)) style = UPDATE;
	else if (! strcmp("-ua", *argv)) style = UPDATE_ALL;
	else if (! strcmp("-o", *argv)) style = OLDER;
	else if (! strcmp("-O", *argv)) style = OLDEST;
	else if (! strcmp("-c", *argv)) style = CMD_LINE;
	else if (! strcmp("-help", *argv)) {
	    (void)fprintf(stderr, 
"TOOLPLACES prints the locations and other relevant information about\n");
	    (void)fprintf(stderr,
"currently active tools.\n");
	    (void)fprintf(stderr, "The options are:\n");
	    (void)fprintf(stderr,
"  -u\tprint updated information only, e.g.,\n");
	    (void)fprintf(stderr,
"\tif icon position information isn't specified when the tool is\n");
	    (void)fprintf(stderr,
"\tstarted then toolplaces won't print the icon's position.\n");
	    (void)fprintf(stderr,
"  -o\tprint the information in the older suntools format.\n");
	    (void)fprintf(stderr,
"  -O\tprint the information in the original toolplaces format without names.\n");
	    (void)fprintf(stderr,
"\nThe default behavior prints all tool information.\n\n");
	}
	else {
	    (void)fprintf(stderr,
		    "Unrecognized option %s.  Usage: %s [ -uoO ] [ -help ]\n",
		    *argv, *(argv - 1));
	    exit(1);
	}
    }
} /* parse_options */


/* 
 * Remove all positioning and sizing information and print the remaining
 * options.
 */
static
print_striped_args(argv)
	char **argv;
{   
    while (*argv != NULL) {
	if (!((strcmp("-Ww", *argv) && strcmp("-width", *argv) &&
	       strcmp("-Wh", *argv) && strcmp("-height", *argv)))) {
	    argv += 2;
	} else if (!((strcmp("-Ws", *argv) && strcmp("-size", *argv) &&
		      strcmp("-Wp", *argv) && strcmp("-position", *argv) &&
		      strcmp("-WP", *argv) && strcmp("-icon_position", *argv)))) {
	    argv += 3;
	} else if (!((strcmp("-Wi", *argv) && strcmp("-iconic", *argv) &&
		      strcmp("-WH", *argv) && strcmp("-help", *argv)))) {
	    argv += 1;
	} else if (!(strcmp("-Wf", *argv) &&
		     strcmp("-foreground_color", *argv) &&
		     strcmp("-Wb", *argv) &&
		     strcmp("-background_color", *argv))) {
	    argv += 4;
	} else {
	    do print_arg(*argv++);
	    while (*argv != NULL && **argv != '-');
	}
    }
    puts("");
} /* print_striped_args */


/* 
 * Search for positioning and sizing options and update their values.
 * If all_info is true added any any missing options to the end of the list.
 */
static
print_updated_args(argv, rect, srect, iconic, all_info)
	char **argv;
	struct rect *rect,  *srect;
	int iconic, all_info;
{   
    int flgp, flgP, flgs, flgi, flgf, flgb;
    flgp = flgP = flgs = flgi = flgf = flgb = 0;
    while (*argv != NULL) {
	if (!((strcmp("-Ww", *argv) && strcmp("-width", *argv) &&
	       strcmp("-Wh", *argv) && strcmp("-height", *argv)))) {
	    argv += 2;
	} else if (!(strcmp("-Wp", *argv) && strcmp("-position", *argv))) {
	    (void)printf("%s %4d %3d ", *argv, rect->r_left, rect->r_top);
	    argv += 3; flgp++;
	} else if (!(strcmp("-Ws", *argv) && strcmp("-size", *argv))) {
	    (void)printf("%s %4d %3d ", *argv, rect->r_width, rect->r_height);
	    argv += 3; flgs++;
	} else if (!(strcmp("-WP", *argv) && strcmp("-icon_position", *argv))) {
	    (void)printf("%s %3d %3d ", *argv, srect->r_left, srect->r_top);
	    argv += 3; flgP++;
	} else if (!(strcmp("-Wi", *argv) && strcmp("-iconic", *argv))) {
	    if (iconic) (void)printf("%s ", *argv);
	    argv += 1; flgi++;
	} else if (!(strcmp("-Wf", *argv) &&
		     strcmp("-foreground_color", *argv))) {
	    argv += 4; flgf++;
	    if (fg_color) (void)printf("%s %3d %3d %3d ", *argv, 
				 cmap_red[1], cmap_green[1], cmap_blue[1]);
	} else if (!(strcmp("-Wb", *argv) &&
		     strcmp("-background_color", *argv))) {
	    argv += 4; flgb++;
	    if (bg_color) (void)printf("%s %3d %3d %3d ", *argv, 
				 cmap_red[0], cmap_green[0], cmap_blue[0]);
	} else if (!(strcmp("-WH", *argv) && strcmp("-help", *argv))) {
	    argv += 1;
	} else {
	    do print_arg(*argv++);
	    while (*argv != NULL && **argv != '-');
	}
    }
    if (all_info) {
	if (!flgp) (void)printf("%s %d %d ", "-Wp", rect->r_left, rect->r_top);
	if (!flgs) (void)printf("%s %d %d ", "-Ws", rect->r_width, rect->r_height);
	if (!flgP) (void)printf("%s %d %d ", "-WP", srect->r_left, srect->r_top);
	if (!flgf && fg_color) (void)printf("-Wf %3d %3d %3d ",
				      cmap_red[0], cmap_green[0], cmap_blue[0]);
	if (!flgb && bg_color) (void)printf("-Wb %3d %3d %3d ",
				      cmap_red[1], cmap_green[1], cmap_blue[1]);
    }	
    if (!flgi && iconic) (void)printf("%s", "-Wi");
    puts("");
} /* print_updated_args */


static
print_arg(arg)
	char *arg;
{
    char *cp;
    int quote;
    
    if (*arg)
	for (cp = arg; *cp; cp++)
	    if (quote = *cp == ' ') break;
    if (quote || *arg == 0)
	(void)printf("\"%s\" ", arg);
    else
	(void)printf("%s ", arg);
}


static
get_colormap_entries(wfd, screen)
	int wfd;
	struct screen *screen;
{   
/*  cms.cms_size = 256;  Win_getcms bug.  Should not have to call it twice */
    bg_color = fg_color = 0;
    cmap.cm_red = cmap.cm_green = cmap.cm_blue = 0;
    (void)win_getcms(wfd, &cms, &cmap); /* Initialize cms_size to be the correct
				   * number of entries. */
    if (cms.cms_size != 2 || strcmp(cms.cms_name, CMS_MONOCHROME) == 0) return;
    cmap.cm_red = cmap_red;
    cmap.cm_green = cmap_green;
    cmap.cm_blue = cmap_blue;
    cms.cms_addr = 0;
    (void)win_getcms(wfd, &cms, &cmap);
    bg_color = (cmap_red[0] != screen->scr_background.red ||
		cmap_green[0] != screen->scr_background.green ||
		cmap_blue[0] != screen->scr_background.blue);
    fg_color = (cmap_red[1] != screen->scr_foreground.red ||
		cmap_green[1] != screen->scr_foreground.green ||
		cmap_blue[1] != screen->scr_foreground.blue);
} /* get_colormap_entries */


static
print_colormap(wfd, screen)
	int wfd;
	struct screen *screen;
{   
    get_colormap_entries(wfd, screen);
    if (bg_color)
	(void)printf("-Wb %3d %3d %3d ", cmap_red[0], cmap_green[0], cmap_blue[0]);
    if (fg_color)
	(void)printf("-Wf %3d %3d %3d ", cmap_red[1], cmap_green[1], cmap_blue[1]);
}
    

/* 
 * Search process table for requested pid and return in argv.
 */
#define AFEW 16
#define ALOT 100

static char **
get_argv(pid)
	short pid;
{   
    static char *argv[ALOT];
    register int i, j;
    struct proc proc[AFEW];		/* 16 = a few, for less syscalls */
    off_t pp;

    argv[0] = NULL;
    if (!pid) return argv;
    pp = procp;	    
    for (i = 0; i < nproc; i += AFEW) {
	(void)lseek(kmem, (long)pp, 0);
	j = nproc - i;
	if (j > AFEW) j = AFEW;
	j *= sizeof (struct proc);
	if (read(kmem, (char *)proc, j) != j) cantread("proc table", kmemf);
	pp += j;
	for (j = j / sizeof (struct proc) - 1; j >= 0; j--)
	    if (proc[j].p_pid == pid) return getcmd(&proc[j], argv);
    }
    argv[0] = "toolname", argv[1] = NULL;
    return argv;
} /* getargv */


/* Open system files and initialize appropriate globals */
init_process_info()
{   
    openfiles();
    getkvars();
    if (chdir("/dev") < 0)   {
#ifndef lint
	error(0, "Can't connect to /dev.");
#endif
    }
} /* init_process_info */


static
openfiles()
{   

    kmemf = "/dev/kmem";
    if ((kmem = open(kmemf, 0)) < 0) {
#ifndef lint
	error1(1, "Can't open %s.", kmemf);
#endif
    }
    memf = "/dev/mem";
    if ((mem = open(memf, 0)) < 0) {
#ifndef lint
	error1(1, "Can't open %s.", memf);
#endif
    }
    swapf = "/dev/drum";
    if ((swap = open(swapf, 0)) < 0) {
#ifndef lint
	error1(1, "Can't open %s.", swapf);
#endif
    }
} /* openfiles */

static
getkvars()
{   

    nlistf = "/vmunix";
    nlist(nlistf, nl);
    if (nl[0].n_type == 0) {
#ifndef lint
	error(0, "No namelist in /vmunix.");
#endif
    }
    usrpt = (struct pte *)nl[X_USRPT].n_value;	/* don''t clear!! */
    Usrptma = (struct pte *)nl[X_USRPTMA].n_value;
    dmmin = getw((FILE *)(LINT_CAST(nl[X_DMMIN].n_value)));
    dmmax = getw((FILE *)(LINT_CAST(nl[X_DMMAX].n_value)));
    nproc = getw((FILE *)(LINT_CAST(nl[X_NPROC].n_value)));
    procp = getw((FILE *)(LINT_CAST(nl[X_PROC].n_value)));
} /* getkvars */


static
getw(loc)
	unsigned long loc;
{   
    long word;

    (void)lseek(kmem, (long)loc, 0);
    if (read(kmem, (char *)&word, sizeof (word)) != sizeof (word))  {
#ifndef lint
	error1(0, "Error reading kmem at %x.", loc);
#endif
}
    return word;
} /* getw */


/* 
 * Walk process structure, format and return the command line.
 */
static char **
getcmd(mproc, argv)
	struct proc *mproc;
	char *argv[];
{   
    union {
	char	argc[CLSIZE*NBPG];
	int	argi[CLSIZE*NBPG/sizeof (int)];
    } argspac;
    register char *cp;
    register int *ip;
    char c;
    int nbad, next = 0;
    struct dblock db;

    if (mproc->p_stat == SZOMB || mproc->p_flag&(SSYS|SWEXIT) ||
	getu(mproc) == 0)
	return argv;
    (void)lseek(kmem, (long)u.u_cred, 0);
    if ((mproc->p_flag & SLOAD) == 0 || argaddr == 0) {
	if (swap < 0) return argv;
	vstodb(0, CLSIZE, &u.u_smap, &db, 1);
	(void) lseek(swap, (long)dtob(db.db_base), 0);
	if (read(swap, (char *)&argspac, sizeof(argspac))
	    != sizeof(argspac))
	    return argv;
    } else {
	(void)lseek(mem, (long)argaddr, 0);
	if (read(mem, (char *)&argspac, sizeof (argspac))
	    != sizeof (argspac))
	    return argv;
    }
    ip = &argspac.argi[CLSIZE*NBPG/sizeof (int)];
    ip -= 2;		/* last arg word and .long 0 */
    while (*--ip)
	if (ip == argspac.argi) return argv;
    *(char *)ip = ' ';
    ip++;
    nbad = 0;
    argv[next++] =  (char *)ip;
    for (cp = (char *)ip; cp < &argspac.argc[CLSIZE*NBPG]; cp++) {
	c = *cp & 0177;
	if (c == 0) argv[next++] = cp + 1;
	else if (c < ' ' || c > 0176) {
	    if (++nbad >= 5) {
		argv[next++] = cp;
		break;
	    }
	    *cp = '?';
	} else if (c == '=') {
	    while (*--cp != ' ')
		if (cp <= (char *)ip)
		    break;
	    break;
	}
    }
    argv[--next] = 0;
    return argv;
} /* getcmd */


static
getu(mproc)
	struct proc *mproc;
{   
    struct pte *pteaddr, apte;
    struct pte arguutl[UPAGES+CLSIZE];
    register int i;
    int ncl, size;

    size = roundup(sizeof (struct user), DEV_BSIZE);
    if ((mproc->p_flag & SLOAD) == 0) {
	if (swap < 0) return 0;
	(void) lseek(swap, (long)dtob(mproc->p_swaddr), 0);
	if (read(swap, (char *)&user.user, size) != size) return 0;
	argaddr = 0;
	return 1;
    }
    pteaddr = &Usrptma[btokmx(mproc->p_p0br) + mproc->p_szpt - 1];
    (void)lseek(kmem, (long)pteaddr, 0);
    if (read(kmem, (char *)&apte, sizeof(apte)) != sizeof(apte)) return 0;
    (void)lseek(mem,
	  (long)ctob(apte.pg_pfnum+1) - (UPAGES+CLSIZE) * sizeof (struct pte),
	  0);
    if (read(mem, (char *)arguutl, sizeof(arguutl)) != sizeof(arguutl))
	return 0;
    if (arguutl[0].pg_fod == 0 && arguutl[0].pg_pfnum)
	argaddr = ctob(arguutl[0].pg_pfnum);
    else
	argaddr = 0;
    ncl = (size + NBPG*CLSIZE - 1) / (NBPG*CLSIZE);
    while (--ncl >= 0) {
	i = ncl * CLSIZE;
	(void)lseek(mem, (long)ctob(arguutl[CLSIZE+i].pg_pfnum), 0);
	if (read(mem, user.upages[i], CLSIZE*NBPG) != CLSIZE*NBPG)
	    return 0;
    }
    return 1;
} /* getu */



/*
 * Given a base/size pair in virtual swap area,
 * return a physical base/size pair which is the
 * (largest) initial, physically contiguous block.
 */
static
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
    while (vsbase >= blk) {
	vsbase -= blk;
	if (blk < dmmax)
	    blk *= 2;
	ip++;
    }
    dbp->db_size = min(vssize, blk - vsbase);
    dbp->db_base = *ip + (rev ? blk - (vsbase + dbp->db_size) : vsbase);
} /* vstodb */


static
cantread(what, fromwhat)
	char *what, *fromwhat;
{   
    (void)fprintf(stderr, "%s: Error reading %s from %s.\n",
	    prog_name, what, fromwhat);
    exit(1);
} /* cantread */
