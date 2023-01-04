# ifdef lint
static char sccsid[] = "@(#)unix_login.c 1.1 86/09/25 Copyr 1985 Sun Micro";
# endif lint

# include <rpc/types.h>
# include <sys/ioctl.h>
# include <sys/signal.h>
# include <sys/file.h>
# include <pwd.h>
# include <errno.h>
# include <stdio.h>
# include <utmp.h>
# include <signal.h>

# include "rex.h"

/*
 * unix_login - hairy junk to simulate logins for Unix
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

char Ttys[] = "/etc/ttys";	/* file to get index of utmp */
char Utmp[] = "/etc/utmp";	/* the tty slots */
char Wtmp[] = "/usr/adm/wtmp";	/* the log information */

int Master, Slave;		/* sides of the pty */
int InputSocket, OutputSocket;	/* Network sockets */
int HelperMask;			/* exported to rexd */
int Helper1, Helper2;		/* pids of the helpers */
char UserName[256];		/* saves the user name for loging */
char HostName[256];		/* saves the host name for loging */
char PtyName[16] = "/dev/ttypn";/* name of the tty we allocated */
static int TtySlot;		/* slot number in Utmp */

/*
 * Check for user being able to run on this machine.
 * returns 0 if OK, TRUE if problem, error message in "error"
 * copies name of shell if user is valid.
 */
ValidUser(host, uid, error, shell)
    char *host;
    int uid;
    char *error;
    char *shell;
{
    struct passwd *pw, *getpwuid();
    
    if (uid == 0) {
    	errprintf(error,"rexd: root execution not allowed\n",uid);
	return(1);
    }
    pw = getpwuid(uid);
    if (pw == NULL) {
    	errprintf(error,"rexd: User id %d not valid\n",uid);
	return(1);
    }
    strncpy(UserName, pw->pw_name, sizeof(UserName)-1 );
    strncpy(HostName, host, sizeof(HostName)-1 );
    strcpy(shell,pw->pw_shell);
    setproctitle(pw->pw_name, host);
    return(0);
}

/*
 *  eliminate any controlling terminal that we have already
 */
NoControl()
{
    int devtty;

    devtty = open("/dev/tty",O_RDWR);
    if (devtty > 0) {
    	    ioctl(devtty, TIOCNOTTY, NULL);
	    close(devtty);
    }
}

/*
 * Allocate a pseudo-terminal
 * sets the global variables Master and Slave.
 * returns 1 on error, 0 if OK
 */
AllocatePty(socket0, socket1)
    int socket0, socket1;
{
# define Sequence "0123456789abcdef"
# define MajorPos 8	/* /dev/ptyXx */
# define MinorPos 9	/* /dev/ptyxX */
# define SidePos 5	/* /dev/Ptyxx */
    static char ptySequence[] = Sequence;
    char maj, min;
    int pgrp;
    int on = 1;

    signal(SIGHUP,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
    for (maj = 'p'; maj <= 's'; maj++)
      for (min = 0; min < strlen(Sequence); min++ ) {
	    PtyName[MajorPos] = maj;
	    PtyName[MinorPos] = ptySequence[min];
	    PtyName[SidePos] = 'p';
	    Master = open(PtyName, O_RDWR);
	    if (Master < 0) {
		continue;
	    }
	    NoControl();
	    PtyName[SidePos] = 't';
	    Slave = open(PtyName, O_RDWR);
	    if (Slave < 0) {
	      perror( PtyName);
	      close(Master);
	      continue;
	    }
	    LoginUser();
	    pgrp = getpid();
	    setpgrp(pgrp, pgrp);
	    InputSocket = socket0;
	    OutputSocket = socket1;
	    ioctl(Master, FIONBIO, &on);
	    HelperMask = (1<<InputSocket) | (1<<Master);
	    return(0);
	}
    /*
     * No pty found!
     */
    return(1);
}


  /*
   * Special processing for interactive operation.
   * Given pointers to three standard file descriptors,
   * which get set to point to the pty.
   */
DoHelper(pfd0, pfd1, pfd2)
    int *pfd0, *pfd1, *pfd2;
{
    int pgrp;

    pgrp = getpid();
    setpgrp(pgrp, pgrp);
    ioctl(Slave, TIOCSPGRP, &pgrp);

    signal( SIGINT, SIG_IGN);
    close(Master);

    *pfd0 = Slave;
    *pfd1 = Slave;
    *pfd2 = Slave;
}


/*
 * destroy the helpers when the executing process dies
 */
KillHelper(grp)
    int grp;
{
    close(Master);
    HelperMask = 0;
    close(InputSocket);
    close(OutputSocket);
    LogoutUser();
    if (grp) killpg(grp,SIGKILL);
    close(Slave);
}


/*
 * edit the Unix traditional data files that tell who is logged
 * into "the system"
 */
LoginUser()
{
  FILE *ttysFile;
  register char *last = PtyName + sizeof("/dev");
  char line[256];
  int count;
  int utf;
  struct utmp utmp;
  
  ttysFile = fopen(Ttys,"r");
  TtySlot = 0;
  count = 0;
  if (ttysFile != NULL) {
      while (fgets(line, sizeof(line), ttysFile) != NULL) {
        register char *lp;
	lp = line + strlen(line) - 1;
	if (*lp == '\n') *lp = '\0';
	count++;
	if (strcmp(last,line+2)==0) {
	  TtySlot = count;
	  break;
	}
      }
      fclose(ttysFile);
  }
  if (TtySlot > 0 && (utf = open(Utmp,O_WRONLY)) >= 0) {
      lseek(utf, TtySlot*sizeof(utmp), L_SET);
      strncpy(utmp.ut_line,last,sizeof(utmp.ut_line));
      strncpy(utmp.ut_name,UserName,sizeof(utmp.ut_name));
      strncpy(utmp.ut_host,HostName,sizeof(utmp.ut_host));
      time(&utmp.ut_time);
      write(utf, (char *)&utmp, sizeof(utmp));
      close(utf);
  }
  if (TtySlot > 0 && (utf = open(Wtmp,O_WRONLY)) >= 0) {
      lseek(utf, (long)0, L_XTND);
      write(utf, (char *)&utmp, sizeof(utmp));
      close(utf);
  }
}


/*
 * edit the Unix traditional data files that tell who is logged
 * into "the system".
 */
LogoutUser()
{
  int utf;
  register char *last = PtyName + sizeof("/dev");
  struct utmp utmp;

  if (TtySlot > 0 && (utf = open(Utmp,O_RDWR)) >= 0) {
      lseek(utf, TtySlot*sizeof(utmp), L_SET);
      read(utf, (char *)&utmp, sizeof(utmp));
      if (strncmp(last,utmp.ut_line,sizeof(utmp.ut_line))==0) {
	lseek(utf, TtySlot*sizeof(utmp), L_SET);
	strcpy(utmp.ut_name,"");
	strcpy(utmp.ut_host,"");
	time(&utmp.ut_time);
	write(utf, (char *)&utmp, sizeof(utmp));
      }
      close(utf);
  }
  if (TtySlot > 0 && (utf = open(Wtmp,O_WRONLY)) >= 0) {
      lseek(utf, (long)0, L_XTND);
      strncpy(utmp.ut_line,last,sizeof(utmp.ut_line));
      write(utf, (char *)&utmp, sizeof(utmp));
      close(utf);
  }
  TtySlot = 0;
}


/*
 * set the pty modes to the given values
 */
SetPtyMode(mode)
    struct rex_ttymode *mode;
{
    int ldisc = NTTYDISC;
    
    ioctl(Slave, TIOCSETD, &ldisc);
    ioctl(Slave, TIOCSETN, &mode->basic);
    ioctl(Slave, TIOCSETC, &mode->more);
    ioctl(Slave, TIOCSLTC, &mode->yetmore);
    ioctl(Slave, TIOCLSET, &mode->andmore);
    
}

/*
 * set the pty window size to the given value
 */
SetPtySize(size)
    struct ttysize *size;
{
    int pgrp;

    (void) ioctl(Slave, TIOCSSIZE, size);
    SendSignal(SIGWINCH);
}


/*
 * send the given signal to the group controlling the terminal
 */
SendSignal(sig)
    int sig;
{
    int pgrp;

    if (ioctl(Slave, TIOCGPGRP, &pgrp) >= 0)
    	(void) killpg( pgrp, sig);
}


/*
 * called when the main select loop detects that we might want to
 * read something.
 */
HelperRead(fds)
    int fds;
{
    char buf[128];
    int cc;
    extern int errno;

    if (fds & (1<<Master)) {
    	cc = read(Master, buf, sizeof buf);
	if (cc > 0)
		(void) write(OutputSocket, buf, cc);
	else {
		if (cc < 0 && errno != EINTR && errno != EWOULDBLOCK)
			perror("pty read");
		shutdown(OutputSocket, 1);
		HelperMask &= ~ (1<<Master);
	}
    }
    if (fds & (1<<InputSocket)) {
    	cc = read(InputSocket, buf, sizeof buf);
	if (cc > 0)
		(void) write(Master, buf, cc);
	else {
		if (cc < 0 && errno != EINTR && errno != EWOULDBLOCK)
			perror("socket read");
		HelperMask &= ~ (1<<InputSocket);
	}
    }
}
