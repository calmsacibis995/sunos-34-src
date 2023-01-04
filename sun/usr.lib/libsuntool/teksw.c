#ifndef lint
static	char sccsid[] = "@(#)teksw.c 1.3 87/01/07 Copyr 1984 Sun Micro";
#endif

/*
 * tek 4014 emulator subwindow interface
 *
 * Author: Steve Kleiman
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <utmp.h>
#include <pwd.h>

#include <pixrect/pixrect.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_struct.h>
#include <suntool/tool.h>
#include <suntool/menu.h>

#include "teksw.h"
#include "tek.h"
#include "teksw_imp.h"

#define TTYSLOT_NOTFOUND(n)		((n)<=0)	/* BSD returns 0; S5 returns -1 */


/*
 * Internal routines
 */
static int ttyinit();
static void fatalperror();
static void fatal();
static int updateutmp();

/*
 * Tool subwindow creation of teksw
 */
struct	toolsw *
teksw_createtoolsubwindow(tool, name, width, height, argv)
struct tool *tool;
char *name;
short width, height;
char **argv;
{
	struct	toolsw *toolsw;

	/*
	 * Create subwindow
	 */
	toolsw = tool_createsubwindow(tool, name, width, height);
	/*
	 * Setup teksw
	 */
	if((toolsw->ts_data = (caddr_t) teksw_init(toolsw->ts_windowfd, argv))
	   == (caddr_t)0)
		return((struct toolsw *)0);
	toolsw->ts_io.tio_handlesigwinch = teksw_handlesigwinch;
	toolsw->ts_io.tio_selected = teksw_selected;
	toolsw->ts_destroy = teksw_done;
	return(toolsw);
}

/*
 * Fork the teksw's driving tty process.
 */
int
teksw_fork(tsd, argv, inputmask, outputmask, exceptmask)
struct	teksubwindow *tsd;
char	**argv;
int	*inputmask, *outputmask, *exceptmask;
{
	register struct teksw *tsp;
	char *args[4];
	char **argstoexec;
	char *shell;
	int childpid;
	extern int errno;
	extern char *getenv();

	tsp = (struct teksw *)tsd;
	childpid = fork();
	if (childpid < 0) {
		fatalperror("");
	}
	if (childpid) {
		int	on = 1;

		/*
		* parent process.
		*/
		ioctl(tsp->windowfd, FIONBIO, &on);
		ioctl(tsp->pty, FIONBIO, &on);
		ioctl(tsp->pty, TIOCPKT, &on);
		signal(SIGTSTP, SIG_IGN);
		/*
		 * the child is going to change its process group to
		 * its process id.
		 */
		tsp->pgrp = childpid;
		*inputmask |= (1<<tsp->pty);
		return(childpid);
	}
	/*
	 * child process.
	 * Change process group so its signal stuff doesn't affect
	 * the terminal emulator.
	 */
	childpid = getpid();
	{ int	(*presigval)() = signal(SIGTTOU, SIG_IGN);

	if ((ioctl(tsp->tty, TIOCSPGRP, &childpid)) == -1)
		perror("teksw_fork - TIOCSPGRP");
	setpgrp(childpid, childpid);
	signal(SIGTTOU, presigval);
	}
	/*
	 * Set up file descriptors
	 */
	close(tsp->windowfd);
	close(tsp->pty);
	dup2(tsp->tty, 0);
	dup2(tsp->tty, 1);
	dup2(tsp->tty, 2);
	close(tsp->tty);
	/*
	 * Determine what shell to run.
	 */
	shell = getenv("SHELL");
	if (!shell || !*shell)
		shell = "/bin/sh";
	args[0] = shell;
	/*
	 * Setup remainder of arguments
	 */
	args[1] = 0;
	argstoexec = args;
	if ((argv != NULL) && (*argv != NULL)) {
		while(*argv){
			if(strcmp("-c", *argv) == 0) {
				/*
				 * The '-c' flag tells the shell to run next arg
				 */
				args[1] = *argv;
				args[2] = *(++argv);
				args[3] = 0;
				break;
			} else if(strcmp("-r", *argv) == 0) {
				/*
				 * The '-r' flag runs the next arg
				 * with next args after that as args
				 */
				argstoexec = ++argv;
				break;
			}
			argv++;
		}
	}
	execvp(*argstoexec, argstoexec);
	/*NOTREACHED*/
	fatalperror(args[0]);
}

/*
 * tek subwindow initialization, destruction and error procedures
 */

struct	teksubwindow *
teksw_init(windowfd, argv)
int windowfd;
char **argv;
{
	register struct teksw *tsp;

	if((tsp = (struct teksw *)calloc(1, sizeof(struct teksw)))
	   == NULL) {
		return((struct teksubwindow *)0);
	}
	tsp->windowfd = windowfd;
	if(!ttyinit(tsp) || !tek_init(tsp, argv))
		return((struct teksubwindow *)0);
	else
		return((struct teksubwindow *)tsp);
}

teksw_done(tsd)
struct	teksubwindow *tsd;
{
	register struct teksw *tsp;

	tsp = (struct teksw *)tsd;
	(void) updateutmp("", tsp->cachedttyslot, tsp->tty);
	tek_done(tsp);
}

/*
 * Internal Routines
 */

/*
 * Do tty/pty setup
 */
static int
ttyinit(tsp)
register struct	teksw *tsp;
{
	int	tt, tmpfd, ptynum = 0;
	char	ptylet = 'p', linebuf[20], *line = &linebuf[0];
	struct	stat stb;

	/*
	 * find unopened pty
	 */
needpty:
	while (ptylet <= 's') {
		strcpy(line, "/dev/ptyXX");
		line[strlen("/dev/pty")] = ptylet;
		line[strlen("/dev/ptyp")] = '0';
		if (stat(line, &stb) < 0)
			break;
		while (ptynum < 16) {
			line[strlen("/dev/ptyp")] = "0123456789abcdef"[ptynum];
			tsp->pty = open(line, 2);
			if (tsp->pty > 0)
				goto gotpty;
			ptynum++;
		}
		ptylet++;
	}
	fatal("All pty's in use");
	/*NOTREACHED*/
gotpty:
	line[strlen("/dev/")] = 't';
	tt = open("/dev/tty", 2);
	if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	}
	tsp->tty = open(line, 2);
	if (tsp->tty < 0) {
		ptynum++;
		close(tsp->pty);
		goto needpty;
	}
	ttysw_restoreparms(tsp->tty);
	/*
	 * Copy stdin.  Set stdin to tty so ttyslot in updateutmp
	 * will think this is the control terminal.  Restore state.
	 * Note: ttyslot should have companion ttyslotf(fd).
	 */
	tmpfd = dup(0);
	close(0);
	dup(tsp->tty);
	tsp->cachedttyslot = updateutmp(0, 0, tsp->tty);
	close(0);
	dup(tmpfd);
	close(tmpfd);
	return(1);
}

/*
 * Fatal error routines
 */
static void
fatalperror(sp)
char *sp;
{
	perror(sp);
	abort();
}

static void
fatal(sp)
char *sp;
{
	fprintf(stderr, sp);
	fprintf(stderr, "\n");
	abort();
}

/*
 * Make entry in /etc/utmp for ttyfd.
 * Note: this is dubious usage of /etc/utmp but many programs (e.g. sccs)
 *   look there when determining who is logged in to the pty.
 */
static int
updateutmp(username, ttyslotuse, ttyfd)
char	*username;
int	ttyslotuse, ttyfd;
{
	/*
	 * Update /etc/utmp
	 */
	struct	utmp utmp;
	struct	passwd *passwdent;
	extern	struct	passwd *getpwuid();
	int	f;
	char	*ttyn;
	extern	char *ttyname(), *rindex();

	if (!username) {
		/*
		 * Get login name
		 */
		if ((passwdent = getpwuid(getuid())) == 0) {
			fatal("couldn't find user name");
		}
		username = passwdent->pw_name;
	}
	utmp.ut_name[0] = '\0'; 	/* Set incase *username is 0 */
	strncpy(utmp.ut_name, username, sizeof(utmp.ut_name));
	/*
	 * Get line (tty) name
	 */
	ttyn = ttyname(ttyfd);
	if (ttyn==(char *)0)
		ttyn = "/dev/tty??";
	strncpy(utmp.ut_line, rindex(ttyn, '/')+1, sizeof(utmp.ut_line));
	/*
	 * Set host to be empty
	 */
	strncpy(utmp.ut_host, "", sizeof(utmp.ut_host));
	/*
	 * Get start time
	 */
	time(&utmp.ut_time);
	/*
	 * Put utmp in /etc/utmp
	 */
	if (ttyslotuse == 0)
		ttyslotuse = ttyslot();
		
	if (TTYSLOT_NOTFOUND(ttyslotuse))
		fatal("ttyslot not found");
		
	if ((f = open("/etc/utmp", 1)) >= 0) {
		lseek(f, (long)(ttyslotuse*sizeof(utmp)), 0);
		write(f, (char *)&utmp, sizeof(utmp));
		close(f);
	} else
		fatalperror("/etc/utmp (make sure that you can write it!)");
		
	return(ttyslotuse);
}
