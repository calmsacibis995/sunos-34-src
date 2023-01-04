#ifndef lint
static	char sccsid[] = "@(#)tape.c 1.1 86/09/25 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include "setup_runtime.h"
#include "install.h"

#define	isremote(ws)	((Tape_loc) setup_get(ws, WS_TAPE_LOC) == TAPE_REMOTE)

typedef	struct	toc	*Toc;
typedef	struct	optsw	*Optsw;

struct	toc 	{
	char	*category;		/* Category name */
	int	tape;			/* Physical tape */
	int	file;			/* Physical file on tape */
};

struct	optsw	{
	Toc	toc;			/* Toc entry */
	Oswg	oswg;			/* Optional software group ptr */
	Optsw	next;			/* Linked list */
};

static	struct	toc	toc[100];	/* Table of contents */
static	int	ntoc;			/* Number of toc entries */
static	char	tapearch[100];		/* Tape contains this architecture */
static	int	tapenum;		/* Number of the tape we're reading */
static	char	tapedev[20];		/* Fullname of tape device */
static	char	tapename[10];		/* Tape controller and unit number */
static	int	tapeunit;		/* Tape unit number */
static	int	blocksize;		/* Block size for this tape */
static	char	tmptocfile[] = "/tmp/toc";	/* Temp toc file */

static	Toc	find_category();
static	Boolean	read_toc();
static	Boolean	istapeup();
static	Optsw	optsw_sort();

/*
 * Read a table of contents from a tape and build a table
 * for internal use.  The table of contents is always file 2.
 */
init_tapetoc(ws, arch)
Workstation	ws;
Arch_served	arch;
{
	FILE		*fp;
	Toc		tp;
	char		name[100];
	char		archstr[100];
	char		*archname;
	char		line[256];

	archname = get_archname(arch, archstr);
	mount_tape(ws, 1, archname);
	fp = fopen(tmptocfile, "r");
	ntoc = 0;
	while (fgets(line, sizeof(line), fp) != NULL) {
		tp = &toc[ntoc];
		sscanf(line, "%d %d %[^\n]", &tp->tape, &tp->file, name);
		tp->category = strdup(name);
		ntoc++;
	}
	fclose(fp);
	unlink(tmptocfile);
}

/*
 * Extract the table of contents and check that it is valid.
 */
static	Boolean
read_toc(ws, tocfile)
Workstation	ws;
char		*tocfile;
{
	FILE		*fp;
	char		cmd[BUF];
	char		line[BUF];

	fsf(ws, 1);
	sprintf(cmd, "dd if=%s > %s", tapedev, tocfile);
	tapecmd(ws, cmd);
	fp = fopen(tocfile, "r");
	if (fp == NULL) {
		runtime_message(SETUP_EOPEN_FAILED, tocfile, "reading");
		return(false);
	}
	if (fgets(line, sizeof(line), fp) == NULL) {
		runtime_message(SETUP_ENO_TOC);
		fclose(fp);
		return(false);
	}
	if (!streq(line, "TABLE OF CONTENTS\n")) {
		runtime_message(SETUP_EBAD_TOC_NOHDR);
		fclose(fp);
		return(false);
	}
	if (fgets(line, sizeof(line), fp) == NULL) {
		runtime_message(SETUP_EBAD_TOC_NOARCH);
		fclose(fp);
		return(false);
	}
	sscanf(line, "%s tape %d", tapearch, &tapenum);
	fclose(fp);
	return(true);
}

/*
 * Find a category in the table of contents.
 */
static	Toc
find_category(ws, category)
Workstation	ws;
char		*category;
{
	Toc		tp;

	for (tp = toc; tp < &toc[ntoc]; tp++) {
		if (streq(category, tp->category)) {
			return(tp);
		}
	}
	runtime_message(SETUP_ENO_OSWG, category);
	message(ws, setup_msgbuf);
	return(NULL);
}

/*
 * See if the tape is correct for the given architecture.
 */
get_tapeup(ws, archname, category)
Workstation	ws;
char		*archname;
char		*category;
{
	int		ntape;
	int		nfile;

	get_tapepos(ws, category, &ntape, &nfile);
	mount_tape(ws, ntape, archname);
	return(nfile);
}

/*
 * Check if a tape is mounted.
 */
static
mount_tape(ws, ntape, archname)
Workstation	ws;
int		ntape;
char		*archname;
{
	char		buf[BUF];

	if (ntape != tapenum || !streq(tapearch, archname)) {
		contproc(ws, ntape, archname);
	}
	for (;;) {
		if (istapeup(ws, tapedev)) {
			rewind(ws);
			if (read_toc(ws, tmptocfile) == true) {
				if (!streq(tapearch, archname)) {
					runtime_message(SETUP_EWRONG_ARCH,
					    archname, tapearch);
				} else if (tapenum != ntape) {
					runtime_message(SETUP_EWRONG_TAPENUM,
					    ntape, tapenum);
				} else {
					break;
				}
			}
		} else {
			runtime_message(SETUP_ENO_TAPE);
		}
		message(ws, setup_msgbuf);
		contproc(ws, ntape, archname);
	} 
}

/*
 * See if the tape is mounted and online.
 * This is done by opening the tape drive.
 * For remote tape installations, this is a real kludge.
 * We use rsh to send over a tape rewind command.
 * If the command failes, it produces an error message
 * and the assumption is that there is no tape present.
 * If the rewind succeeds then there is no error message.
 */
static	Boolean
istapeup(ws, tapedev)
Workstation	ws;
char		*tapedev;
{
	Boolean		r;
	FILE		*pfp;
	char		cmd[BUF];
	char		line[BUF];
	int		fd;

	if (isremote(ws)) {
		sprintf(cmd, "rsh %s -n 'sh -c \"mt -f %s rew 2>&1\"'",
			setup_get(ws, WS_TAPE_SERVER), tapedev);
		close_on_exec();
		pfp = popen(cmd, "r");
		if (fgets(line, sizeof(line), pfp) != NULL) {
			r = false;
		} else {
			r = true;
		}
		pclose(pfp);
	} else {
		fd = open(tapedev, 0);
		if (fd != -1) {
			close(fd);
			r = true;
		} else {
			r = false;
		}
	}
	return(r);
}

/*
 * Call the continuation proc.
 * This procedure posts a message and waits for an indication from
 * the user that he is ready to proceed.
 */
static
contproc(ws, ntape, archname)
Workstation	ws;
int		ntape;
char		*archname;
{
	Voidfunc	proc;
	char		buf[BUF];

	proc = (Voidfunc) setup_get(ws, SETUP_CONTINUE_PROC);
	sprintf(buf, "Please mount tape %d for architecture `%s'",
	    ntape, archname);
	if (proc == NULL) {
		message(ws, buf);
		sleep(30);
	} else {
		proc(buf);
	}
}

/*
 * Given a category, return its tape number and tape file number.
 */
static
get_tapepos(ws, category, tapenum, tapefile)
Workstation	ws;
char		*category;
int		*tapenum;
int		*tapefile;
{
	Toc		tp;

	tp = find_category(ws, category);
	if (tapenum != NULL) {
		*tapenum = tp->tape;
	}
	if (tapefile != NULL) {
		*tapefile = tp->file;
	}
}

/*
 * Install the optional software.
 * Go thru all the selected software groups and sort them
 * according to their tape number and position.
 */
install_optsoftware(ws, arch)
Workstation	ws;
Arch_served	arch;
{
	Workstation_type ws_type;
	Optsw		optsw;
	Optsw		op;
	char		archstr[BUF];
	char		*archname;
	char		*dir;
	int		tape;

	ws_type = (Workstation_type) setup_get(ws, WS_TYPE);
	optsw = optsw_sort(ws, arch);
	if (optsw != NULL) {
		archname = get_archname(arch, archstr);
		get_tapeup(ws, archname, optsw->toc->category);
		tape = optsw->toc->tape;
		for (op = optsw; op != NULL; op = op->next) {
			if (op->toc->tape != tape) {
				tape = op->toc->tape;
				get_tapeup(ws, archname, op->toc->category);
			} 
			dir = setup_get(op->oswg, OSWG_DIRECTORY, (int)ws_type);
			extract(ws, arch, op->toc, dir);
		}
	}
}

/*
 * Sort the selected optional software groups by tape and
 * file position.
 */
static	Optsw
optsw_sort(ws, arch)
Workstation	ws;
Arch_served	arch;
{
	Oswg		oswg;
	Optsw		optsw;
	Toc		tp;
	char		*oswg_name;
	int		ix;

	optsw = NULL;
	SETUP_FOREACH_OBJECT(arch, ARCH_OSWG_SELECTED, ix, oswg) {
		oswg_name = setup_get(oswg, OSWG_NAME);
		tp = find_category(ws, oswg_name);
		optsw_insert(&optsw, oswg, tp);
	} SETUP_END_FOREACH
	return(optsw);
}

/*
 * Insert a new optional software group into a sorted list
 */
static
optsw_insert(bpatch, oswg, tp)
Optsw	*bpatch;
Oswg	oswg;
Toc	tp;
{
	Optsw		optsw;
	Optsw		op;

	optsw = new(struct optsw);
	optsw->toc = tp;
	optsw->oswg = oswg;
	optsw->next = NULL;
	for (op = *bpatch; op != NULL; op = op->next) {
		if (tp->tape < op->toc->tape ||
		    (tp->tape == op->toc->tape && tp->file < op->toc->file)) {
			break;
		}
		bpatch = &op->next;
	}
	optsw->next = op;
	*bpatch = optsw;
}

/*
 * Extract the /usr/sys (Kernel source) files.
 */
xtr_sys(ws, arch)
Workstation	ws;
Arch_served	arch;
{
	Toc		toc;
	char		archstr[BUF];
	char		*archname;

	archname = get_archname(arch, archstr);
	toc = find_category(ws, "Sys");
	get_tapeup(ws, archname, toc->category);
	extract(ws, arch, toc, "/usr");
}

/*
 * Extract the files for an optional software category.
 * Currently, play it safe and always rewind the tape and fsf
 * out to the correct file.  Should try to minimize tape motion
 * but one of the SCSI tape controllers doesn't do the right things.
 * All optional software is on the tape with path names relative
 * to /usr.  Here we cd to /usr to extract.  For server's this
 * means cd'ing to the /usr of the right architecture.
 */
static
extract(ws, arch, toc, oswg_dir)
Workstation	ws;
Arch_served	arch;
Toc		toc;
char		*oswg_dir;
{
	char		cmd[BUF];
	char		dir[BUF];
	char		archstr[BUF];
	char		*archname;

	archname = get_archname(arch, archstr);
	if (isserver(ws)) {
		sprintf(dir, "%s%s.%s", ROOT_DIR, oswg_dir, archname);
	} else {
		sprintf(dir, "%s%s", ROOT_DIR, oswg_dir);
	}
	chdir(dir);
	rewind(ws);
	fsf(ws, toc->file - 1);
	sprintf(cmd, "Extracting `%s'", toc->category);
	message(ws, cmd);
	if (isremote(ws)) {
		sprintf(cmd, "rsh %s -n dd if=%s bs=%db | tar xfpB -",
		    setup_get(ws, WS_TAPE_SERVER), tapedev, blocksize);
	} else {
		sprintf(cmd, "tar xfbp %s %d", tapedev, blocksize);
	}
	mysystem(ws, cmd);
	chdir("/");
	rewind(ws);
}

/*
 * Rewind a tape.
 */
static
rewind(ws)
Workstation	ws;
{
	char		cmd[BUF];
	int		r;

	sprintf(cmd, "mt -f %s rew", tapedev);
	r = tapecmd(ws, cmd);
	return(r);
}

/*
 * Forward skip n tape files.
 */
static
fsf(ws, nfiles)
Workstation	ws;
int		nfiles;
{
	char		cmd[BUF];
	int		r;

	r = 0;
	if (nfiles > 0) {
		sprintf(cmd, "mt -f %s fsf %d", tapedev, nfiles);
		r = tapecmd(ws, cmd);
	}
	return(r);
}

/*
 * Issue a tape command.
 * If we have a remote tape server, then an 'rsh' command
 * must be used.
 */
static
tapecmd(ws, cmd)
Workstation	ws;
char		*cmd;
{
	char		newcmd[BUF];
	char		*cmdp;
	int		r;

	if (isremote(ws)) {
		sprintf(newcmd, "rsh %s -n %s", 
		    setup_get(ws, WS_TAPE_SERVER), cmd);
		cmdp = newcmd;
	} else {
		cmdp = cmd;
	}
	r = mysystem(ws, cmdp);
	return(r);
}

/*
 * Get the tape device name, unit number and possible tape
 * server.
 */
get_tape(ws, serv_arch)
Workstation	ws;
Arch_served	serv_arch;
{
	FILE		*fp;
	char		cmd[BUF];
	char		archstr[BUF];
	char		tape[10];
	char		*archname;
	char		*string;
	int		tape_ix;
	int		len;

	tape_ix = (int) setup_get(ws, WS_TAPE_TYPE);
	string = setup_get(ws, SETUP_CHOICE_STRING, CONFIG_TAPETYPE, tape_ix);
	strcpy(tapename, get_device_abbrev(string));
	strcpy(tape, tapename);
	len = strlen(tape);
	tapeunit = atoi(&tape[len - 1]);
	tape[len - 1] = '\0';
	if (isremote(ws)) {
		fp = fopen(HOSTS, "a");
		if (fp == NULL) {
			error(ws, SETUP_EOPEN_FAILED, HOSTS, "appending");
		}
		fprintf(fp, "%s.%s\t%s\n", 
		    setup_get(ws, WS_NETWORK),
		    setup_get(ws, WS_HOST_NUMBER),
		    setup_get(ws, WS_NAME));
		fprintf(fp, "%s\t%s\n", 
		    setup_get(ws, WS_HOST_INTERNET_NUMBER), 
		    setup_get(ws, WS_TAPE_SERVER));
		fclose(fp);
		ifconfig(ws);
	} else {
		sprintf(cmd, "(cd /dev; ./MAKEDEV %s)", tapename);
		mysystem(ws, cmd);
	}

	/*
	 * Total kludge here, change the xt device
	 * to an mt one.  Also assign block sizes according
	 * to the device name.
	 */
	if (streq(tape, "xt")) {
		strcpy(tape, "mt");
	}
	if (streq(tape, "mt")) {
		blocksize = BS_1OVER2;
	} else {
		blocksize = BS_1OVER4;
	}
	sprintf(tapedev, "/dev/nr%s%d", tape, tapeunit);

	tapenum = 1;
	archname = get_archname(serv_arch, archstr);
	strcpy(tapearch, archname);
}

/*
 * Return the unit number of the tape.
 */
get_tapeunit()
{
	return(tapeunit);
}

/*
 * Return the tape block size.
 */
get_blocksize()
{
	return(blocksize);
}

/*
 * Return the full name of the tape device
 */
char	*
get_tapedev()
{
	return(tapedev);
}

/*
 * Return the tape name (controller and unit number)
 */
char	*
get_tapename()
{
	return(tapename);
}

/*
 * If using a remote tape server, return the string "tapeserver=name"
 * otherwise return a null string.
 */
char	*
get_remote(ws)
Workstation	ws;
{
	static	char	buf[BUF];

	if (isremote(ws)) {
		sprintf(buf, "tapeserver=%s", setup_get(ws, WS_TAPE_SERVER));
		return(buf);
	} else {
		return("");
	}
}

/*
 * See if there is a tape mounted on the drive that has been chosen.
 * If so everything is ok.  If not, request that the tape be mount
 * and after confirmation see if it is there.  If is still missing
 * assume there is a problem.
 */
Boolean
tapedev_ok(ws, serv_arch)
Workstation	ws;
Arch_served	serv_arch;
{
	char		archstr[BUF];
	char		*archname;

	if (istapeup(ws, tapedev)) {
		return(true);
	}
	archname = get_archname(serv_arch, archstr);
	contproc(ws, 1, archname);
	if (istapeup(ws, tapedev)) {
		return(true);
	}
	return(false);
}
